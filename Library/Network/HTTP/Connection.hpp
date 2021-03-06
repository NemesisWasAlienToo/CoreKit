#pragma once

#include <string>
#include <tuple>
#include <memory>
#include <variant>
#include <future>

#include <Event.hpp>
#include <Duration.hpp>
#include <ePoll.hpp>
#include <Iterable/List.hpp>
#include <Network/HTTP/HTTP.hpp>
#include <Format/Stream.hpp>
#include <Network/HTTP/Response.hpp>
#include <Network/HTTP/Request.hpp>
#include <Network/HTTP/Parser.hpp>
#include <Network/TCPServer.hpp>
#include <Network/HTTP/Router.hpp>
#include <File.hpp>

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
                    std::shared_ptr<File> FilePtr;
                    size_t FileContentLength;
                    bool SendFile;
                };

                struct Context : public Async::EventLoop::Context
                {
                    Network::EndPoint const &Target;

                    inline void SendResponse(HTTP::Response &&Response) const
                    {
                        Loop.AssertPersmission();

                        Self.CallbackAs<HTTP::Connection>()->AppendResponse(std::move(Response));

                        ListenFor(ePoll::In | ePoll::Out);
                    }

                    template <typename TCallback>
                    inline void OnRemove(TCallback &&Callback)
                    {
                        HandlerAs<HTTP::Connection>().OnRemove = std::forward<TCallback>(Callback);
                    }

                    template <typename TCallback>
                    inline void OnIdle(TCallback &&Callback)
                    {
                        HandlerAs<HTTP::Connection>().OnIdle = std::forward<TCallback>(Callback);
                    }

                    template <typename TCallback>
                    inline void Upgrade(TCallback &&Callback, ePoll::Event Events = ePoll::In)
                    {
                        ListenFor(ePoll::Out);

                        OnIdle(
                            [*this, Events, Callback = std::forward<TCallback>(Callback)]() mutable
                            {
                                ListenFor(Events);

                                Self.Callback = std::forward<TCallback>(Callback);
                            });
                    }

                    // inline void InsertHandler();
                };

                struct Settings
                {
                    size_t MaxHeaderSize;
                    size_t MaxBodySize;
                    size_t MaxFileSize;
                    size_t SendFileThreshold;
                    size_t RequestBufferSize;
                    size_t ResponseBufferSize;
                    std::string HostName;
                    std::function<void(Context &, Network::HTTP::Response &)> OnError;
                    std::function<std::optional<Network::HTTP::Response>(Context &, Network::HTTP::Request &)> OnRequest;
                };

                Network::EndPoint Target;
                Duration Timeout;
                Iterable::Queue<HTTP::Request> IBuffer;
                Iterable::Queue<OutEntry> OBuffer;
                Settings const &Setting;

                // Events
                std::function<void()> OnRemove;
                std::function<void()> OnIdle;

                // @todo Fix this limitations
                HTTP::Parser Parser{Setting.MaxHeaderSize, Setting.MaxBodySize, Setting.RequestBufferSize};
                bool ShouldClose = false;

                Connection(Network::EndPoint const &Target, Duration const &Timeout, Settings &setting)
                    : Target(Target),
                      Timeout(Timeout),
                      Setting(setting)
                {
                }

                ~Connection()
                {
                    if (OnRemove)
                        OnRemove();
                }

                void AppendResponse(HTTP::Response &&Response)
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

                    bool HasFile = std::holds_alternative<std::shared_ptr<File>>(Response.Content);
                    size_t FileLength = 0;
                    size_t StringLength = 0;

                    HasFile ? FileLength =
                                  std::min(std::get<std::shared_ptr<File>>(Response.Content)->Size(), Setting.MaxFileSize)
                            : StringLength = std::min(std::get<std::string>(Response.Content).length(), Setting.MaxBodySize);

                    bool UseSendFile = Setting.SendFileThreshold && HasFile && FileLength > Setting.SendFileThreshold;

                    // Trim content if its too big

                    Response.Headers.insert_or_assign("Content-Length", std::to_string(HasFile ? FileLength : StringLength));
                    Response.Headers.insert_or_assign("Host", Setting.HostName);

                    OBuffer.Insert(
                        {Iterable::Queue<char>(Setting.ResponseBufferSize),
                         // @todo Remove pointer
                         HasFile ? std::get<std::shared_ptr<File>>(Response.Content) : nullptr,
                         FileLength,
                         UseSendFile});

                    auto &Item = OBuffer.Tail();

                    Format::Stream Ser(Item.Buffer);

                    Ser << Response;

                    // Append file content

                    if (Item.SendFile)
                        return;

                    while (Item.FileContentLength)
                    {
                        Item.FileContentLength -= Ser.ReadOnce(*Item.FilePtr, Item.FileContentLength);
                    }
                }

                void operator()(Async::EventLoop::Context &Context, ePoll::Entry &Item)
                {
                    Network::Socket &Client = *static_cast<Network::Socket *>(&Context.Self.File);
                    Connection::Context ConnContext{Context, Target};

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

                        Context.Reschedual(Timeout);

                        OnRead(ConnContext);
                    }

                    if (Item.Happened(ePoll::Out))
                    {
                        OnWrite(ConnContext);
                    }
                }

                void OnRead(Connection::Context &Context)
                {
                    Network::Socket &Client = *static_cast<Network::Socket *>(&Context.Self.File);

                    try
                    {
                        Parser(Client);

                        if (Parser.IsFinished())
                        {
                            // Process request

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
                                    ShouldClose = true;
                                    Client.ShutDown(Network::Socket::Read);
                                }
                            }

                            // Append response to buffer

                            if (auto Result = Setting.OnRequest(Context, Parser.Result))
                            {
                                Context.SendResponse(std::move(Result.value()));
                            }

                            if (ShouldClose)
                                return;

                            // Reset Parser

                            Parser.Reset();
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
                    }
                }

                void OnWrite(Connection::Context &Context)
                {
                    Network::Socket &Client = *static_cast<Network::Socket *>(&Context.Self.File);

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

                        if (OnIdle)
                            OnIdle();

                        return;
                    }

                    auto &Item = OBuffer.Head();
                    Format::Stream Ser(Item.Buffer);

                    // Send data in buffer

                    if (!Item.Buffer.IsEmpty())
                    {
                        // Write data

                        // @todo Make socket non-blocking and improve this

                        Context.Self.File << Ser;

                        if (!Item.Buffer.IsEmpty())
                            return;
                    }

                    // Send file

                    if (Item.FileContentLength && Item.SendFile)
                    {
                        Item.FileContentLength -= Client.SendFile(*Item.FilePtr, Item.FileContentLength);
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