#pragma once

#include <string>

#include <File.hpp>
#include <Duration.hpp>
#include <Format/Stream.hpp>
#include <Network/HTTP/Response.hpp>
#include <Network/HTTP/Request.hpp>
#include <Network/TLSContext.hpp>
#include <Network/HTTP/Parser.hpp>

namespace Core
{
    namespace Network
    {
        namespace HTTP
        {
            struct Connection
            {
                struct OutEntry
                {
                    Iterable::Queue<char> Buffer;
                    File FilePtr;
                    size_t FileContentLength;
                };

                struct Context : public Async::EventLoop::Context
                {
                    Network::EndPoint const &Target;
                    Network::EndPoint const &Source;

                    inline bool IsSecure()
                    {
                        return Self.CallbackAs<HTTP::Connection>()->IsSecure();
                    }

                    inline bool HasKTLS()
                    {
                        return false;
                    }

                    inline bool CanUseSendFile()
                    {
                        auto s = IsSecure();
                        return !s || (s && HasKTLS());
                    }

                    inline std::nullopt_t SendResponse(HTTP::Response const &Response, File file = {}, size_t FileLength = 0) const
                    {
                        Loop.AssertPermission();

                        Self.CallbackAs<HTTP::Connection>()->AppendResponse(Response, std::move(file), FileLength);

                        ListenFor(ePoll::In | ePoll::Out);

                        return std::nullopt;
                    }

                    inline std::nullopt_t SendBuffer(Iterable::Queue<char> Buffer, File file = {}, size_t FileLength = 0) const
                    {
                        Loop.AssertPermission();

                        Self.CallbackAs<HTTP::Connection>()->AppendBuffer(std::move(Buffer), std::move(file), FileLength);

                        ListenFor(ePoll::In | ePoll::Out);

                        return std::nullopt;
                    }

                    inline bool WillClose()
                    {
                        return HandlerAs<HTTP::Connection>().ShouldClose;
                    }

                    template <typename TCallback>
                    inline void OnRemove(TCallback &&Callback)
                    {
                        HandlerAs<HTTP::Connection>().OnRemove = std::forward<TCallback>(Callback);
                    }

                    template <typename TCallback>
                    inline void OnReceived(TCallback &&Callback)
                    {
                        HandlerAs<HTTP::Connection>().OnReceived = std::forward<TCallback>(Callback);
                    }

                    template <typename TCallback>
                    inline void OnSent(TCallback &&Callback)
                    {
                        HandlerAs<HTTP::Connection>().OnSent = std::forward<TCallback>(Callback);
                    }
                };

                struct Settings
                {
                    size_t MaxHeaderSize;
                    size_t MaxBodySize;
                    size_t RequestBufferSize;
                    size_t ResponseBufferSize;
                    std::function<void(Context &, Network::HTTP::Response &)> OnError;
                    std::function<std::optional<Network::HTTP::Response>(Context &, Network::HTTP::Request &)> OnRequest;
                    size_t MaxConnectionCount;
                    bool NoDelay;
                    Duration Timeout;
                };

                Network::EndPoint Target;
                Network::EndPoint Source;
                Iterable::Queue<char> IBuffer;
                Iterable::Queue<OutEntry> OBuffer;
                Settings const &Setting;
                TLSContext::SecureSocket SSL;

                // Events
                std::function<void()> OnRemove;
                std::function<void()> OnReceived;
                std::function<void()> OnSent;

                // @todo Fix this limitations
                HTTP::Parser Parser{Setting.MaxHeaderSize, Setting.MaxBodySize, Setting.RequestBufferSize, IBuffer};
                bool ShouldClose = false;

                Connection(Network::EndPoint const &target, Network::EndPoint const &source, Settings &setting)
                    : Target(target),
                      Source(source),
                      Setting(setting),
                      SSL()
                {
                }

                // TLS Connection

                Connection(Network::EndPoint const &target, Network::EndPoint const &source, Settings &setting, TLSContext::SecureSocket &&SS)
                    : Target(target),
                      Source(source),
                      Setting(setting),
                      SSL(std::move(SS))
                {
                }

                Connection(Connection &&Other) : Target(Other.Target),
                                                 Source(Other.Source),
                                                 IBuffer(std::move(Other.IBuffer)),
                                                 OBuffer(std::move(Other.OBuffer)),
                                                 Setting(Other.Setting),
                                                 SSL(std::move(Other.SSL))
                {
                }

                ~Connection()
                {
                    if (OnRemove)
                        OnRemove();
                }

                inline bool IsSecure()
                {
                    return bool(SSL);
                }

                void Continue100(Connection::Context &Context)
                {
                    Network::Socket &Client = static_cast<Network::Socket &>(Context.Self.File);

                    // Ensure version is HTTP 1.1

                    if (Parser.Result.Version[5] == '1' && Parser.Result.Version[7] == '0')
                    {
                        throw HTTP::Status::ExpectationFailed;
                    }

                    // Validate Content-Length

                    if (!Parser.HasBody())
                    {
                        throw HTTP::Status::LengthRequired;
                    }

                    // Respond to 100-continue

                    // @todo Maybe handle HTTP 2.0 later too?
                    // @todo Fix this and use actual request

                    constexpr auto ContinueResponse = "HTTP/1.1 100 Continue\r\n\r\n";

                    Iterable::Queue<char> Temp(sizeof(ContinueResponse), false);
                    Format::Stream Stream(Temp);

                    Temp.CopyFrom(ContinueResponse, sizeof(Stream));

                    // Send the response

                    while (Temp.Length())
                    {
                        // (bool(SSL) ? SSL : Client) << Stream;

                        if (SSL)
                        {
                            if (!(SSL << Stream))
                            {
                                Context.Remove();
                                return;
                            }
                        }
                        else
                        {
                            Client << Stream;
                        }
                    }
                }

                inline void AppendBuffer(Iterable::Queue<char> Buffer, File file = {}, size_t FileLength = 0)
                {
                    OBuffer.Insert({std::move(Buffer), std::move(file), FileLength});
                }

                void AppendResponse(HTTP::Response const &Response, File file = {}, size_t FileLength = 0)
                {
                    [[maybe_unused]] size_t StringLength = 0;
                    auto Buffer = Iterable::Queue<char>(Setting.ResponseBufferSize);
                    Format::Stream Ser(Buffer);

                    // Serialize first line

                    Ser << "HTTP/" << Response.Version << ' ' << std::to_string(static_cast<unsigned short>(Response.Status)) << ' ' << Response.Brief << "\r\n";

                    // Serialize headers
                    
                    for (auto const &[k, v] : Response.Headers)
                        Ser << k << ':' << v << "\r\n";

                    // Handle keep-alive

                    if (!this->ShouldClose && Response.Version == HTTP::HTTP10)
                    {
                        Ser << "Connection:keep-alive\r\n";
                    }
                    else if (this->ShouldClose && Response.Version == HTTP::HTTP11)
                    {
                        Ser << "Connection:close\r\n";
                    }

                    if (file)
                    {
                        FileLength = FileLength ? FileLength : file.BytesLeft();
                    }
                    else
                    {
                        StringLength = Response.Content.length();
                    }

                    Ser << "Content-Length:" << std::to_string(FileLength + StringLength) << "\r\n";

                    Response.SetCookies.ForEach(
                        [&](auto const &Cookie)
                        {
                            Ser << "Set-Cookie:" << Cookie << "\r\n";
                        });

                    Ser << "\r\n" << Response.Content;

                    AppendBuffer(std::move(Buffer), std::move(file), FileLength);
                }

                void operator()(Async::EventLoop::Context &Context, ePoll::Entry &Item)
                {
                    Network::Socket &Client = static_cast<Network::Socket &>(Context.Self.File);
                    Connection::Context ConnContext{Context, Target, Source};

                    if (Item.Happened(ePoll::HangUp) || Item.Happened(ePoll::Error))
                    {
                        Context.Remove();
                        return;
                    }

                    if (Item.Happened(ePoll::In) || Item.Happened(ePoll::UrgentIn))
                    {
                        if (!Client.Received())
                        {
                            Context.Remove();
                            return;
                        }

                        bool Exit = false;

                        OnRead(ConnContext, Exit);

                        if (Exit)
                        {
                            return;
                        }
                    }

                    if (Item.Happened(ePoll::Out))
                    {
                        OnWrite(ConnContext);
                    }
                }

                void OnRead(Connection::Context &Context, bool &Exit)
                {
                    Network::Socket &Client = static_cast<Network::Socket &>(Context.Self.File);

                    try
                    {
                        Format::Stream Stream(IBuffer);

                        if (SSL)
                        {
                            if (!(SSL >> Stream))
                            {
                                Exit = true;
                                return;
                            }
                        }
                        else
                        {
                            Client >> Stream;
                        }

                        // Client >> Stream;

                        Context.Reschedule(Setting.Timeout);

                        Parser();

                        if (Parser.RequiresContinue100)
                        {
                            Continue100(Context);
                            return;
                        }
                    }
                    catch (HTTP::Status Method)
                    {
                        auto Response = HTTP::Response::From(Parser.Result.Version.empty() ? HTTP10 : Parser.Result.Version, Method, {{"Connection", "close"}}, "");

                        if (Setting.OnError)
                            Setting.OnError(Context, Response);

                        AppendResponse(Response);

                        Context.ListenFor(ePoll::Out);

                        ShouldClose = true;

                        return;
                    }

                    if (!Parser.IsFinished())
                        return;

                    // Decide if we should keep the connection

                    auto It = Parser.Result.Headers.find("Connection");
                    auto End = Parser.Result.Headers.end();

                    // @todo Optimize this

                    {
                        std::string ConnectionValue;

                        if (It != End)
                        {
                            // @todo Optimize this

                            ConnectionValue.resize(It->second.length());

                            std::transform(
                                It->second.begin(),
                                It->second.end(),
                                ConnectionValue.begin(),
                                [](auto c)
                                {
                                    return std::tolower(c);
                                });
                        }

                        // @todo Optimize this

                        if ((Parser.Result.Version == HTTP::HTTP10 && ConnectionValue != "keep-alive") ||
                            (Parser.Result.Version == HTTP::HTTP11 && ConnectionValue == "close"))
                        {
                            Client.ShutDown(Network::Socket::ShutdownRead);
                            ShouldClose = true;
                        }
                    }

                    // Append response to buffer

                    if (auto Result = Setting.OnRequest(Context, Parser.Result))
                    {
                        AppendResponse(Result.value());

                        Context.ListenFor(ePoll::In | ePoll::Out);

                        // Same as bellow

                        // Context.SendResponse(std::move(Result.value()));

                        if (OnReceived)
                            OnReceived();
                    }

                    if (ShouldClose)
                        return;

                    // Reset Parser

                    Parser.Reset();
                }

                void OnWrite(Connection::Context &Context)
                {
                    Network::Socket &Client = static_cast<Network::Socket &>(Context.Self.File);

                    // If there is nothing to send

                    if (OBuffer.IsEmpty())
                    {
                        if (ShouldClose)
                        {
                            Context.Remove();
                            return;
                        }

                        OBuffer.Free();

                        Context.ListenFor(ePoll::In);

                        if (OnSent)
                            OnSent();

                        return;
                    }

                    auto &Item = OBuffer.Head();
                    Format::Stream Stream(Item.Buffer);

                    // Send data in buffer

                    if (!Item.Buffer.IsEmpty())
                    {
                        // Write data

                        // bool(SSL) ? SSL << Stream : Client << Stream;

                        if (SSL)
                        {
                            if (!(SSL << Stream))
                            {
                                Context.Remove();
                                return;
                            }
                        }
                        else
                        {
                            Client << Stream;
                        }

                        // Client << Stream;

                        if (!Item.Buffer.IsEmpty())
                            return;
                    }

                    // Send file

                    if (Item.FileContentLength)
                    {
                        if (SSL)
                            Item.FileContentLength -= SSL.SendFile(Item.FilePtr, Item.FileContentLength);
                        else
                            Item.FileContentLength -= Client.SendFile(Item.FilePtr, Item.FileContentLength);
                    }

                    // Pop buffer if we're done

                    if (Item.Buffer.IsEmpty() && !Item.FileContentLength)
                    {
                        OBuffer.Take();
                    }
                }
            };
        }
    }
}