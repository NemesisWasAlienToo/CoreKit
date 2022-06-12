#pragma once

#include <string>

#include <Event.hpp>
#include <Duration.hpp>
#include <Poll.hpp>
#include <ePoll.hpp>
#include <Iterable/List.hpp>
#include <Network/HTTP/HTTP.hpp>
#include <Format/Serializer.hpp>
#include <Network/HTTP/Response.hpp>
#include <Network/HTTP/Request.hpp>
#include <Network/HTTP/Router.hpp>
#include <Network/HTTP/Parser.hpp>
#include <Network/TCPServer.hpp>

// @todo Use timout

namespace Core
{
    namespace Network
    {
        namespace HTTP
        {
            class Server
            {
            public:
                Server() = default;
                Server(EndPoint const &endPoint, Duration const &timeout, size_t ThreadCount = std::thread::hardware_concurrency(), Duration const &Interval = Duration::FromMilliseconds(500))
                    : TCP(
                        //   endPoint, [this](auto &Loop)
                        //   { return BuildClientHandler(Loop); },
                          endPoint, [this]()
                          { return BuildClientHandler(); },
                          timeout,
                          ThreadCount),
                      Timeout(timeout)
                {
                }

                void Run()
                {
                    TCP.Run();
                }

                void GetInPool()
                {
                    TCP.GetInPool();
                }

                void Stop()
                {
                    TCP.Stop();
                }

                inline void SetDefault(const std::string &Pattern, HTTP::Router::Handler Action)
                {
                    Router.Default(
                        HTTP::Router::Route::From(
                            HTTP::Request::Methods::Any,
                            Pattern,
                            std::move(Action)));
                }

                inline void Set(HTTP::Request::Methods Method, const std::string &Pattern, HTTP::Router::Handler Action, std::string Extension = "" /*, Validator Filter = nullptr*/)
                {
                    Router.Add(
                        HTTP::Router::Route::From(
                            Method,
                            Pattern,
                            std::move(Action),
                            std::move(Extension)));
                }

                inline void GET(const std::string &Pattern, HTTP::Router::Handler Action, std::string Extension = "" /*, Validator Filter = nullptr*/)
                {
                    Set(
                        HTTP::Request::Methods::GET,
                        Pattern,
                        std::move(Action),
                        std::move(Extension));
                }

                inline void POST(const std::string &Pattern, HTTP::Router::Handler Action, std::string Extension = "" /*, Validator Filter = nullptr*/)
                {
                    Set(
                        HTTP::Request::Methods::POST,
                        Pattern,
                        std::move(Action),
                        std::move(Extension));
                }

                inline void PUT(const std::string &Pattern, HTTP::Router::Handler Action, std::string Extension = "" /*, Validator Filter = nullptr*/)
                {
                    Set(
                        HTTP::Request::Methods::PUT,
                        Pattern,
                        std::move(Action),
                        std::move(Extension));
                }

                inline void DELETE(const std::string &Pattern, HTTP::Router::Handler Action, std::string Extension = "" /*, Validator Filter = nullptr*/)
                {
                    Set(
                        HTTP::Request::Methods::DELETE,
                        Pattern,
                        std::move(Action),
                        std::move(Extension));
                }

                inline void HEAD(const std::string &Pattern, HTTP::Router::Handler Action, std::string Extension = "" /*, Validator Filter = nullptr*/)
                {
                    Set(
                        HTTP::Request::Methods::HEAD,
                        Pattern,
                        std::move(Action),
                        std::move(Extension));
                }

                inline void OPTIONS(const std::string &Pattern, HTTP::Router::Handler Action, std::string Extension = "" /*, Validator Filter = nullptr*/)
                {
                    Set(
                        HTTP::Request::Methods::OPTIONS,
                        Pattern,
                        std::move(Action),
                        std::move(Extension));
                }

                inline void PATCH(const std::string &Pattern, HTTP::Router::Handler Action, std::string Extension = "" /*, Validator Filter = nullptr*/)
                {
                    Set(
                        HTTP::Request::Methods::PATCH,
                        Pattern,
                        std::move(Action),
                        std::move(Extension));
                }

                inline void Any(const std::string &Pattern, HTTP::Router::Handler Action, std::string Extension = "" /*, Validator Filter = nullptr*/)
                {
                    Set(
                        HTTP::Request::Methods::Any,
                        Pattern,
                        std::move(Action),
                        std::move(Extension));
                }

                Async::EventLoop::CallbackType BuildClientHandler()
                {
                    return [&, ShouldClose = false](Async::EventLoop *Loop, ePoll::Entry &Item, Async::EventLoop::Entry &Self) mutable
                    {
                        if (Item.Happened(ePoll::In))
                        {
                            // Read data

                            Network::Socket &Client = *static_cast<Network::Socket *>(&Self.File);

                            if (!Client.Received())
                            {
                                Loop->Remove(Self.Iterator);
                                return;
                            }

                            try
                            {
                                if (Self.Parser.IsStarted())
                                {
                                    Self.Parser.Continue();
                                }
                                else
                                {
                                    Self.Parser.Start(&Client);
                                }

                                if (Self.Parser.IsFinished())
                                {
                                    // Process request
                                    
                                    // @todo Optimize Info passing

                                    Network::HTTP::Response Response = OnRequest(Client.Peer(), Self.Parser.Result);

                                    auto It = Self.Parser.Result.Headers.find("Connection");
                                    auto End = Self.Parser.Result.Headers.end();

                                    // @todo Fix case sensitivity of header values

                                    if (Self.Parser.Result.Version == HTTP::HTTP10)
                                    {
                                        if ((It != End && It->second == "Keep-Alive"))
                                        {

                                            Response.Headers.insert(std::make_pair("Connection", "Keep-Alive"));
                                        }
                                        else
                                        {
                                            ShouldClose = true;
                                        }
                                    }
                                    else if (Self.Parser.Result.Version == HTTP::HTTP11 && It != End && It->second == "close")
                                    {
                                        ShouldClose = true;
                                    }

                                    // Append response to buffer

                                    Response.AppendToBuffer(Self.Buffer);

                                    // Modify events

                                    Loop->Modify(Self, ePoll::In | ePoll::Out);

                                    // Reset Parser

                                    Self.Parser.Reset();
                                }
                            }
                            catch (HTTP::Response const &Response)
                            {
                                Response.AppendToBuffer(Self.Buffer);

                                ShouldClose = true;
                            }
                        }

                        if (Item.Happened(ePoll::Out))
                        {
                            // Check if has data to send

                            if (Self.Buffer.IsEmpty())
                            {
                                // If not stop listenning to out event

                                if (ShouldClose)
                                {
                                    Loop->Remove(Self.Iterator);
                                    return;
                                }

                                Self.Buffer.Free();

                                Loop->Modify(Self, ePoll::In);

                                // @todo Optimize this

                                Loop->Reschedual(Self, Timeout);

                                return;
                            }

                            // Write data

                            Format::Stream Ser(Self.Buffer);

                            // Make socket non-blocking and improve this

                            Self.File << Ser;
                        }

                        if (Item.Happened(ePoll::HangUp) || Item.Happened(ePoll::Error))
                        {
                            Loop->Remove(Self.Iterator);
                        }
                    };
                }

            private:
                TCPServer TCP;
                HTTP::Router Router;
                Duration Timeout;

                HTTP::Response OnRequest(Network::EndPoint const &Target, Network::HTTP::Request &Request)
                {
                    // Search in routes for match

                    auto [Route, Parameters] = Router.Match(Request.Type, Request.Path);

                    return Route.Action(Target, Request, Parameters);
                }
            };
        }
    }
}