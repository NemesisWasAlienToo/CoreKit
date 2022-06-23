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
            template <typename TServer>
            struct ConnectionHandler
            {
                struct OutEntry
                {
                    Iterable::Queue<char> Buffer;
                    std::shared_ptr<File> FilePtr;
                    size_t FileContentLength;
                    bool SendFile;
                };

                Duration Timeout;
                Network::EndPoint Target;
                // Iterable::Queue<char> IBuffer;
                Iterable::Queue<OutEntry> OBuffer;
                TServer &CTServer;

                // @todo Fix this limitations
                HTTP::Parser Parser{CTServer.Settings.MaxHeaderSize, CTServer.Settings.MaxBodySize};
                bool ShouldClose = false;

                ConnectionHandler(Duration const &Timeout, Network::EndPoint const &Target, TServer &Server)
                    : Timeout(Timeout),
                      Target(Target),
                      CTServer(Server)
                {
                }

                void AppendResponse(HTTP::Response &Response)
                {
                    bool HasFile = std::holds_alternative<std::shared_ptr<File>>(Response.Content);
                    size_t FileLength = 0;
                    size_t StringLength = 0;

                    HasFile ? FileLength =
                                  std::min(std::get<std::shared_ptr<File>>(Response.Content)->Size(), CTServer.Settings.MaxFileSize)
                            : StringLength = std::min(std::get<std::string>(Response.Content).length(), CTServer.Settings.MaxBodySize);

                    bool UseSendFile = CTServer.Settings.SendFileThreshold && HasFile && FileLength > CTServer.Settings.SendFileThreshold;

                    // Trim content if its too big

                    Response.Headers.insert_or_assign("Content-Length", std::to_string(HasFile ? FileLength : StringLength));
                    Response.Headers.insert_or_assign("Host", CTServer.Settings.HostName);

                    OBuffer.Add(
                    // OBuffer.Construct(
                        Iterable::Queue<char>(CTServer.Settings.RequestBufferSize),
                        // @todo Remove pointer
                        HasFile ? std::get<std::shared_ptr<File>>(Response.Content) : nullptr,
                        FileLength,
                        UseSendFile);

                    Format::Stream Ser(OBuffer.Tail().Buffer);

                    Ser << Response;
                }

                void operator()(Async::EventLoop *Loop, ePoll::Entry &Item, Async::EventLoop::Entry &Self)
                {
                    Network::Socket &Client = *static_cast<Network::Socket *>(&Self.File);

                    if (Item.Happened(ePoll::In) | Item.Happened(ePoll::UrgentIn))
                    {
                        if (!Client.Received())
                        {
                            Loop->Remove(Self.Iterator);
                            return;
                        }

                        Loop->Reschedual(Self, Timeout);

                        OnRead(Loop, Item, Self);
                    }

                    if (Item.Happened(ePoll::Out))
                    {
                        OnWrite(Loop, Item, Self);
                    }

                    if (Item.Happened(ePoll::HangUp) || Item.Happened(ePoll::Error))
                    {
                        Loop->Remove(Self.Iterator);
                    }
                }

                void OnRead(Async::EventLoop *Loop, ePoll::Entry &, Async::EventLoop::Entry &Self)
                {
                    Network::Socket &Client = *static_cast<Network::Socket *>(&Self.File);

                    try
                    {
                        if (Parser.IsStarted())
                        {
                            Parser.Continue();
                        }
                        else
                        {
                            Parser.Start(&Client);
                        }

                        if (Parser.IsFinished())
                        {
                            // Process request

                            // @todo Maybe check Host tag?

                            Network::HTTP::Response Response = CTServer.OnRequest(Target, Parser.Result, Loop->Storage);

                            // Decide if we should keep the connection

                            auto It = Parser.Result.Headers.find("Connection");
                            auto End = Parser.Result.Headers.end();

                            // @todo Fix case sensitivity of header values

                            if (Parser.Result.Version == HTTP::HTTP10)
                            {
                                if ((It != End && It->second == "Keep-Alive"))
                                {

                                    Response.Headers.insert_or_assign("Connection", "Keep-Alive");
                                }
                                else
                                {
                                    ShouldClose = true;
                                }
                            }
                            else if (Parser.Result.Version == HTTP::HTTP11 && It != End && It->second == "close")
                            {
                                ShouldClose = true;
                            }

                            // If we should close the connection, stop reading data from client

                            if (ShouldClose)
                            {
                                Client.ShutDown(Network::Socket::Read);
                            }

                            // Append response to buffer

                            AppendResponse(Response);

                            // Modify events

                            Loop->Modify(Self, ShouldClose ? ePoll::Out : ePoll::In | ePoll::Out);

                            // Reset Parser

                            Parser.Reset();
                        }
                    }
                    catch (HTTP::Response &Response)
                    {
                        if (CTServer.Settings.OnError)
                            CTServer.Settings.OnError(Target, Response, Loop->Storage);

                        AppendResponse(Response);

                        // Set tcp no push if enabled bu user

                        Loop->Modify(Self, ShouldClose ? ePoll::Out : ePoll::In | ePoll::Out);

                        ShouldClose = true;
                    }
                    catch (HTTP::Status Method)
                    {
                        auto Response = HTTP::Response::From(Parser.Result.Version.empty() ? HTTP10 : Parser.Result.Version, Method, {{"Connection", "close"}}, "");

                        // if (CTServer.Settings.OnError)
                        //     CTServer.Settings.OnError(Target, Response, Loop->Storage);

                        AppendResponse(Response);

                        Loop->Modify(Self, ShouldClose ? ePoll::Out : ePoll::In | ePoll::Out);

                        ShouldClose = true;
                    }
                }

                void OnWrite(Async::EventLoop *Loop, ePoll::Entry &, Async::EventLoop::Entry &Self)
                {
                    Network::Socket &Client = *static_cast<Network::Socket *>(&Self.File);

                    // If there is nothing to send

                    if (OBuffer.IsEmpty())
                    {
                        if (ShouldClose)
                        {
                            Loop->Remove(Self.Iterator);
                            return;
                        }

                        OBuffer.Free();

                        Loop->Modify(Self, ePoll::In);
                        return;
                    }

                    auto &Item = OBuffer.Head();
                    Format::Stream Ser(Item.Buffer);

                    // Append file content

                    if (Item.FileContentLength && !Item.SendFile)
                    {
                        Item.FileContentLength -= Ser.ReadOnce(*Item.FilePtr, Item.FileContentLength);
                    }

                    // Send data in buffer

                    if (!Item.Buffer.IsEmpty())
                    {
                        // Write data

                        // @todo Make socket non-blocking and improve this

                        Self.File << Ser;

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