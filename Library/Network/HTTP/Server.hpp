#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <mutex>

#include <Event.hpp>
#include <Duration.hpp>
#include <Iterable/Poll.hpp>
#include <Iterable/List.hpp>
#include <Network/Socket.hpp>
#include <Network/HTTP/HTTP.hpp>
#include <Format/Serializer.hpp>
#include <Network/HTTP/Response.hpp>
#include <Network/HTTP/Request.hpp>
#include <Network/HTTP/Router.hpp>

namespace Core
{
    namespace Network
    {
        namespace HTTP
        {
            class Server
            {
            private:
                enum class States : char
                {
                    Stopped = 0,
                    Running,
                };

                std::mutex Lock;
                Network::Socket Socket;
                Event Interrupt;

                Iterable::Poll Poll;

                Network::EndPoint Host;
                Duration TimeOut;
                volatile States State;

                Iterable::Span<std::thread> Pool;

                HTTP::Router Router;

                void HandleClient(Network::Socket Client, Network::EndPoint Info)
                {
                    std::string Request;

                    size_t ContetLength = 0;
                    size_t lenPos = 0;

                    size_t bodyPos = 0;
                    size_t bodyPosTmp = 0;

                    while (true)
                    {
                        // Await for data

                        Client.Await(Network::Socket::In, TimeOut.AsMilliseconds());

                        size_t Received = Client.Received();

                        if (Received == 0)
                        {
                            Client.Close();
                            return;
                        }

                        // Read received data

                        char RequestBuffer[Received];

                        Client.Receive(RequestBuffer, Received);

                        Request.append(RequestBuffer, Received);

                        // Detect content lenght

                        if (ContetLength == 0 && bodyPos == 0)
                        {
                            lenPos = Request.find("Content-Length: ", lenPos - 15);

                            if (lenPos == std::string::npos)
                            {
                                lenPos = Request.length();
                            }
                            else
                            {
                                std::string len = Request.substr(lenPos + 16);
                                len = len.substr(0, len.find("\r\n"));

                                ContetLength = std::stoul(len);
                            }
                        }

                        // Detect start of content

                        if (bodyPos == 0)
                        {
                            bodyPosTmp = Request.find("\r\n\r\n", bodyPosTmp);

                            if (bodyPosTmp == std::string::npos)
                            {
                                bodyPosTmp = Request.length() - 3;
                            }
                            else
                            {
                                bodyPos = bodyPosTmp + 4;
                            }
                        }

                        if (bodyPos && (Request.length() - bodyPos) >= ContetLength)
                            break;
                    }

                    // Parse request

                    Core::Network::HTTP::Request Req = Core::Network::HTTP::Request::From(Request, bodyPos - 4);

                    // Handle request and form response

                    Core::Network::HTTP::Response Res = OnRequest(Info, Req);

                    // Serialize response

                    std::string ResponseText = Res.ToString();

                    Core::Iterable::Queue<char> Buffer(ResponseText.length());
                    Buffer.Add(ResponseText.c_str(), ResponseText.length());

                    Format::Serializer Ser(Buffer);

                    // Send response

                    while (!Buffer.IsEmpty())
                    {
                        Client << Ser;

                        Client.Await(Core::Network::Socket::Out);
                    }

                    Client.Close();
                }

                HTTP::Response OnRequest(const Network::EndPoint &Target, Network::HTTP::Request &Request)
                {
                    // Search in routes for match

                    auto [Route, Parameters] = Router.Match(Request.Type, Request.Path);

                    return Route.Action(Target, Request, Parameters);
                }

            public:
                Server(const Network::EndPoint &Host, Duration Timout) : Socket(Network::Socket::IPv4, Network::Socket::TCP), Host(Host), TimeOut(Timout), State(States::Stopped)
                {
                    Socket.Bind(Host);

                    Poll.Add(Socket, Network::Socket::In);
                    Poll.Add(Interrupt, Event::In);
                }

                ~Server()
                {
                    Socket.Close();
                }

                // Functions

                void SetDefault(const std::string &Pattern, HTTP::Router::Handler Action)
                {
                    Router.Default(
                        HTTP::Router::Route::From(
                            HTTP::Request::Methods::Any,
                            Pattern,
                            std::move(Action)));
                }

                void Set(HTTP::Request::Methods Method, const std::string &Pattern, HTTP::Router::Handler Action /*, Validator Filter = nullptr*/)
                {
                    Router.Add(
                        HTTP::Router::Route::From(
                            Method,
                            Pattern,
                            std::move(Action)));
                }

                void GET(const std::string &Pattern, HTTP::Router::Handler Action /*, Validator Filter = nullptr*/)
                {
                    Set(
                        HTTP::Request::Methods::GET,
                        Pattern,
                        std::move(Action));
                }

                void POST(const std::string &Pattern, HTTP::Router::Handler Action /*, Validator Filter = nullptr*/)
                {
                    Set(
                        HTTP::Request::Methods::POST,
                        Pattern,
                        std::move(Action));
                }

                void PUT(const std::string &Pattern, HTTP::Router::Handler Action /*, Validator Filter = nullptr*/)
                {
                    Set(
                        HTTP::Request::Methods::PUT,
                        Pattern,
                        std::move(Action));
                }

                void DELETE(const std::string &Pattern, HTTP::Router::Handler Action /*, Validator Filter = nullptr*/)
                {
                    Set(
                        HTTP::Request::Methods::DELETE,
                        Pattern,
                        std::move(Action));
                }

                void HEAD(const std::string &Pattern, HTTP::Router::Handler Action /*, Validator Filter = nullptr*/)
                {
                    Set(
                        HTTP::Request::Methods::HEAD,
                        Pattern,
                        std::move(Action));
                }

                void OPTIONS(const std::string &Pattern, HTTP::Router::Handler Action /*, Validator Filter = nullptr*/)
                {
                    Set(
                        HTTP::Request::Methods::OPTIONS,
                        Pattern,
                        std::move(Action));
                }

                void PATCH(const std::string &Pattern, HTTP::Router::Handler Action /*, Validator Filter = nullptr*/)
                {
                    Set(
                        HTTP::Request::Methods::PATCH,
                        Pattern,
                        std::move(Action));
                }

                void TRACE(const std::string &Pattern, HTTP::Router::Handler Action /*, Validator Filter = nullptr*/)
                {
                    Set(
                        HTTP::Request::Methods::TRACE,
                        Pattern,
                        std::move(Action));
                }

                void CONNECT(const std::string &Pattern, HTTP::Router::Handler Action /*, Validator Filter = nullptr*/)
                {
                    Set(
                        HTTP::Request::Methods::CONNECT,
                        Pattern,
                        std::move(Action));
                }

                void Any(const std::string &Pattern, HTTP::Router::Handler Action /*, Validator Filter = nullptr*/)
                {
                    Set(
                        HTTP::Request::Methods::Any,
                        Pattern,
                        std::move(Action));
                }

                // Startup functions

                HTTP::Server& Listen(int Max)
                {
                    Socket.Listen(Max);
                    return *this;
                }

                void GetInPool()
                {
                    while (true)
                    {
                        Lock.lock();

                        Poll();

                        if (Poll[0].HasEvent())
                        {
                            auto [Client, Info] = Socket.Accept();

                            Lock.unlock();

                            HandleClient(Client, Info);
                        }
                        else if (Poll[1].HasEvent())
                        {
                            Lock.unlock();
                            break;
                        }
                    }
                }

                HTTP::Server& Start(int Count = std::thread::hardware_concurrency())
                {
                    Pool = Iterable::Span<std::thread>(Count);

                    State = States::Running;

                    for (size_t i = 0; i < Pool.Length(); i++)
                    {
                        Pool[i] = std::thread(
                            [this]
                            {
                                GetInPool();
                            });
                    }
                }

                void Stop()
                {
                    State = States::Stopped;

                    Interrupt.Emit(1);

                    // Join threads in pool

                    Pool.ForEach(
                        [](std::thread &Thread)
                        {
                            Thread.join();
                        });

                    Interrupt.Listen();
                }
            };
        };
    }
}