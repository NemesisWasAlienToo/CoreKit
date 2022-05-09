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
#include <Network/HTTP/Parser.hpp>

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
                    HTTP::Parser parser;

                    // @todo catch(const HTTP::Response& Response)

                    // @todo Optimize the loop

                    Client.Await(Network::Socket::In);

                    parser.Start(Client);

                    while (!parser.IsFinished())
                    {
                        Client.Await(Network::Socket::In);

                        parser.Continue();
                    }
                    
                    // Handle request and form response

                    Network::HTTP::Response Res = OnRequest(Info, parser.Result);

                    // Serialize response

                    Iterable::Queue<char> Buffer = Res.ToBuffer();

                    Format::Stringifier Ser(Buffer);

                    // Send response

                    while (!Buffer.IsEmpty())
                    {
                        Client << Ser;

                        Client.Await(Core::Network::Socket::Out);
                    }

                    // Check if connection should be closed

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

                // Startup functions

                HTTP::Server &Listen(int Max)
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

                HTTP::Server &Start(int Count = std::thread::hardware_concurrency())
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

                    return *this;
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