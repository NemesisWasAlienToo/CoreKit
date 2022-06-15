#pragma once

#include <string>
#include <tuple>
#include <memory>
#include <variant>

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
                Duration Timeout;
                Network::EndPoint Target;
                // Iterable::Queue<char> IBuffer;
                Iterable::Queue<std::tuple<Iterable::Queue<char>, std::shared_ptr<File>>> OBuffer;
                TServer &CTServer;

                HTTP::Parser Parser;
                bool ShouldClose = false;

                ConnectionHandler(Duration const &Timeout, Network::EndPoint const &Target, TServer &Server)
                    : Timeout(Timeout),
                      Target(Target),
                      CTServer(Server)
                {
                }

                void AppendResponse(HTTP::Response const &Response)
                {
                    OBuffer.Add(
                        {Iterable::Queue<char>(1024),
                         std::holds_alternative<std::shared_ptr<File>>(Response.Content) ? std::get<std::shared_ptr<File>>(Response.Content) : nullptr});

                    Format::Stream Ser(std::get<0>(OBuffer.Last()));

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

                            // @todo Optimize Info passing

                            Network::HTTP::Response Response = CTServer.OnRequest(Target, Parser.Result);

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
                    catch (HTTP::Response const &Response)
                    {
                        AppendResponse(Response);

                        ShouldClose = true;
                    }
                }

                void OnWrite(Async::EventLoop *Loop, ePoll::Entry &, Async::EventLoop::Entry &Self)
                {
                    Network::Socket &Client = *static_cast<Network::Socket *>(&Self.File);

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

                    auto &CurBuf = OBuffer.First();

                    // Write data

                    Format::Stream Ser(std::get<0>(CurBuf));

                    // Make socket non-blocking and improve this

                    Self.File << Ser;

                    if (Ser.Queue.IsEmpty())
                    {
                        if (std::get<1>(CurBuf))
                        {
                            // @todo Optimize getting size

                            std::get<1>(CurBuf)->Seek();
                            Client.SendFile(*std::get<1>(CurBuf), std::get<1>(CurBuf)->Size());
                        }

                        OBuffer.Take();
                    }
                }
            };
        }
    }
}