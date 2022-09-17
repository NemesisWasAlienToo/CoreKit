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
                        return HandlerAs<HTTP::Connection>().IsSecure();
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

                    inline void SendResponse(HTTP::Response const &Response, File file = {}, size_t FileLength = 0) const
                    {
                        Loop.AssertPermission();

                        HandlerAs<HTTP::Connection>().AppendResponse(Response, std::move(file), FileLength);

                        ListenFor(ePoll::In | ePoll::Out);
                    }

                    inline void SendBuffer(Iterable::Queue<char> Buffer, File file = {}, size_t FileLength = 0) const
                    {
                        Loop.AssertPermission();

                        HandlerAs<HTTP::Connection>().AppendBuffer(std::move(Buffer), std::move(file), FileLength);

                        ListenFor(ePoll::In | ePoll::Out);
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
                    Core::Function<void(Context &, Network::HTTP::Response &)> OnError;
                    Core::Function<void(Context &, Network::HTTP::Request &)> OnRequest;
                    size_t MaxConnectionCount;
                    bool NoDelay;
                    bool RawContent;
                    Duration Timeout;
                };

                Network::EndPoint Target;
                Network::EndPoint Source;

                Iterable::Queue<char> IBuffer = Iterable::Queue<char>(1);
                Iterable::Queue<OutEntry> OBuffer = Iterable::Queue<OutEntry>(1);
                Settings const &Setting;
                TLSContext::SecureSocket SSL;

                // Events
                Core::Function<void()> OnRemove;
                Core::Function<void()> OnReceived;
                Core::Function<void()> OnSent;

                // @todo Fix this limitations
                HTTP::Parser<HTTP::Request> Parser{Setting.MaxHeaderSize, Setting.MaxBodySize, Setting.RequestBufferSize, IBuffer, Setting.RawContent};
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

                bool Continue100(Connection::Context &Context)
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
                        if ((SSL ? SSL.Write(Stream) : Client.Write(Stream)) <= 0)
                        {
                            return false;
                        }
                    }

                    return true;
                }

                inline void AppendBuffer(Iterable::Queue<char> Buffer, File file = {}, size_t FileLength = 0)
                {
                    OBuffer.Insert({std::move(Buffer), std::move(file), FileLength});
                }

                void AppendResponse(HTTP::Response const &Response, File file = {}, size_t FileLength = 0)
                {
                    size_t StringLength = 0;
                    auto Buffer = Iterable::Queue<char>(Setting.ResponseBufferSize);
                    Format::Stream Ser(Buffer);

                    // Serialize first line

                    Ser << "HTTP/" << Response.Version << ' ' << std::to_string(static_cast<unsigned short>(Response.Status)) << ' ' << Response.Brief << "\r\n";

                    // Serialize headers

                    for (auto const &[k, v] : Response.Headers)
                        Ser << k << ": " << v << "\r\n";

                    // Handle keep-alive

                    if (!this->ShouldClose && Response.Version == HTTP::HTTP10)
                    {
                        Ser << "connection: keep-alive\r\n";
                    }
                    else if (this->ShouldClose && Response.Version == HTTP::HTTP11)
                    {
                        Ser << "connection: close\r\n";
                    }

                    // Calculate length

                    if (file)
                    {
                        FileLength = FileLength ? FileLength : file.BytesLeft();
                    }
                    else
                    {
                        StringLength = Response.Content.length();
                    }

                    if (Response.Headers.find("content-length") == Response.Headers.end())
                        Ser << "content-length: " << std::to_string(FileLength + StringLength) << "\r\n";

                    Response.SetCookies.ForEach(
                        [&](auto const &Cookie)
                        {
                            Ser << "set-cookie: " << Cookie << "\r\n";
                        });

                    Ser << "\r\n"
                        << Response.Content;

                    AppendBuffer(std::move(Buffer), std::move(file), FileLength);
                }
                
                void operator()(Async::EventLoop::Context &Context, ePoll::Entry &Item)
                {
                    Connection::Context ConnContext{Context, Target, Source};

                    if (Item.Happened(ePoll::HangUp) || Item.Happened(ePoll::Error) ||
                        ((Item.Happened(ePoll::In) || Item.Happened(ePoll::UrgentIn)) && !OnRead(ConnContext)) ||
                        (Item.Happened(ePoll::Out) && !OnWrite(ConnContext)))
                    {
                        Context.Remove();
                        return;
                    }

                    Context.Reschedule(Setting.Timeout);
                }

                bool OnRead(Connection::Context &Context)
                {
                    Network::Socket &Client = static_cast<Network::Socket &>(Context.Self.File);

                    // @todo Optimize parser by giving it parsing error callbacks so we
                    // dont need try catch block

                    try
                    {
                        Format::Stream Stream(IBuffer);

                        static constexpr size_t Threshold = 1024 * 2;
                        size_t Free = Stream.Queue.IsFree();

                        if (Free < Threshold)
                            Stream.Queue.IncreaseCapacity(Threshold - Free);

                        if ((SSL ? SSL.Read(Stream) : Client.Read(Stream)) <= 0)
                        {
                            return false;
                        }

                        Parser();

                        if (Parser.RequiresContinue100)
                        {
                            return Continue100(Context);
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

                        return true;
                    }

                    if (!Parser.IsFinished())
                        return true;

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

                    Setting.OnRequest(Context, Parser.Result);

                    if (OnReceived)
                        OnReceived();

                    if (!ShouldClose)
                        Parser.Reset();

                    return true;
                }

                bool OnWrite(Connection::Context &Context)
                {
                    Network::Socket &Client = static_cast<Network::Socket &>(Context.Self.File);

                    // If there is nothing to send

                    if (OBuffer.IsEmpty())
                    {
                        if (ShouldClose)
                        {
                            return false;
                        }

                        OBuffer.Free();
                        Context.ListenFor(ePoll::In);

                        if (OnSent)
                            OnSent();

                        return true;
                    }

                    auto &Item = OBuffer.Head();
                    Format::Stream Stream(Item.Buffer);

                    // Send data in buffer

                    if (!Item.Buffer.IsEmpty())
                    {
                        // Write data

                        if ((SSL ? SSL.Write(Stream) : Client.Write(Stream)) <= 0)
                        {
                            return false;
                        }

                        if (!Item.Buffer.IsEmpty())
                            return true;
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

                    return true;
                }
            };
        }
    }
}