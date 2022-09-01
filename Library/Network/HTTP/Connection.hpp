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

                    inline std::nullopt_t SendResponse(HTTP::Response &&Response, File file = {}, size_t FileLength = 0) const
                    {
                        Loop.AssertPermission();

                        Self.CallbackAs<HTTP::Connection>()->AppendResponse(std::move(Response), std::move(file), FileLength);

                        ListenFor(ePoll::In | ePoll::Out);

                        return std::nullopt;
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

                    template <typename TCallback>
                    inline void Upgrade(TCallback &&Callback, Duration Timeout /*, ePoll::Event Events = ePoll::In*/)
                    {
                        Loop.Upgrade(Self, std::forward<TCallback>(Callback), Timeout);
                    }

                    // inline void InsertHandler();
                };

                struct Settings
                {
                    size_t MaxHeaderSize;
                    size_t MaxBodySize;
                    size_t MaxFileSize;
                    size_t RequestBufferSize;
                    size_t ResponseBufferSize;
                    std::string HostName;
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

                void AppendResponse(HTTP::Response &&Response, File file = {}, size_t FileLength = 0)
                {
                    // Handle keep-alive

                    if (!ShouldClose && Response.Version == HTTP::HTTP10)
                    {
                        Response.Headers.insert_or_assign("Connection", "keep-alive");
                    }
                    else if (ShouldClose && Response.Version == HTTP::HTTP11)
                    {
                        Response.Headers.insert_or_assign("Connection", "close");
                    }

                    bool HasFile = bool(file);
                    size_t StringLength = 0;

                    // Trim content if its too big
                    // @todo What should we do if content is too large? Trim? Exception? or HTTP::Status::RequestedRangeNotSatisfiable

                    if (HasFile)
                    {
                        FileLength = (FileLength && Setting.MaxFileSize) ? FileLength : std::min(file.Size(), Setting.MaxFileSize);

                        // if (Offset)
                        //     file.Seek(Offset);
                    }
                    else
                    {
                        StringLength = std::min(Response.Content.length(), Setting.MaxBodySize);
                    }

                    Response.Headers.insert_or_assign("Content-Length", std::to_string(FileLength + StringLength));
                    Response.Headers.insert_or_assign("Host", Setting.HostName);

                    OBuffer.Insert(
                        {Iterable::Queue<char>(Setting.ResponseBufferSize),
                         // @todo Remove pointer
                         HasFile ? std::move(file) : std::move(File{}),
                         FileLength});

                    auto &Item = OBuffer.Tail();

                    Format::Stream Ser(Item.Buffer);

                    Ser << Response;
                }

                void Handshake(Connection::Context &Context)
                {
                    // @todo Maybe do this with upgrade?

                    auto Result = SSL.Handshake();

                    if (Result == 1)
                    {
                        SSL.ShakeHand = true;
                        Context.ListenFor(ePoll::In);
                        return;
                    }

                    auto Error = SSL.GetError(Result);

                    if (Error == SSL_ERROR_WANT_WRITE)
                    {
                        Context.ListenFor(ePoll::Out | ePoll::In);
                    }
                    else if (Error == SSL_ERROR_WANT_READ)
                    {
                        Context.ListenFor(ePoll::In);
                    }
                    else
                    {
                        Context.Remove();
                    }
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

                    if (SSL && !SSL.ShakeHand)
                    {
                        Handshake(ConnContext);
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
                            Handshake(ConnContext);
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

                        AppendResponse(std::move(Response));

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
                        AppendResponse(std::move(Result.value()));

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