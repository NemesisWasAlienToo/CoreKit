/**
 * @file Runner.cpp
 * @author Nemesis (github.com/NemesisWasAlienToo)
 * @brief DHT node runner
 *
 *      Supported operations: [ Response, Invalid, Ping, Query, Set, Get, Data ]
 *
 *      The packet format for each operation is as follows :
 *
 *      Response   : [ [ Header - Size ] - My id ] - Response Specifier - Data
 *      Invalid    : [ [ Header - Size ] - My id ] - Invalid Specifier - Cause of failure
 *      Ping       : [ [ Header - Size ] - My id ] - Ping Specifier - My ID
 *      Query      : [ [ Header - Size ] - My id ] - Query Specifier - Target ID
 *      Set        : [ [ Header - Size ] - My id ] - Set Specifier - Key - Data
 *      Get        : [ [ Header - Size ] - My id ] - Get Specifier - Key
 *      Data       : [ [ Header - Size ] - My id ] - Data Specifier - Data
 *
 * @todo Optimize adding and removing node by using binary tree or linked list
 * @todo Network::DHT::Runner<Network::DHT::Chord> Chrod syntax
 * @todo Nodes mutex and lock
 * @todo Optimization
 * @version 0.1
 * @date 2022-01-03
 *
 * @copyright Copyright (c) 2022
 *
 */

#pragma once

#include <iostream>
#include <string>
#include <thread>

#include "Network/EndPoint.cpp"
#include "Network/DHT/Node.cpp"
#include "Network/Server.cpp"
#include "Network/DHT/Cache.cpp"
#include "Network/Handler.cpp"
#include "Iterable/List.cpp"
#include "Iterable/Span.cpp"
#include "Iterable/Poll.cpp"
#include "Format/Serializer.cpp"
#include "Duration.cpp"
#include "Test.cpp"

namespace Core
{
    namespace Network
    {
        class Runner
        {
        public:
            enum class Operations : char
            {
                Response = 0,
                Invalid,
                Ping,
                Query,
                Set,
                Get,
                Data,
            };

        private:
            // ### Types

            // ### Variables

            enum class States : char
            {
                Stopped = 0,
                Running,
            };

            DHT::Node Identity;

            Network::Server Server;

            Network::Handler Handler;

            Event Interrupt;

            std::mutex Lock;

            Iterable::Poll Poll;

            Iterable::List<std::thread> Pool;

            DHT::Cache Cache;

            States State;

            void Await()
            {
                Poll[0].Mask = Server.Events();
                // Poll[1].Mask = Timer::In; // Handler
                // Poll[2].Mask = Event::In; // Interrupt

                Poll();
            }

            void EventLoop()
            {
                while (State == States::Running)
                {
                    // @todo Clean this

                    Lock.lock();

                calc:
                    Await();

                    if (Poll[0].HasEvent())
                    {
                        if (Poll[0].Happened(Iterable::Poll::Out))
                        {
                            Server.Send();
                            Lock.unlock();
                        }
                        else if (Poll[0].Happened(Iterable::Poll::In))
                        {
                            if (!Server.Take(

                                    // Take the completed request

                                    [this](Request &request)
                                    {
                                        // After taing out the request free the lock

                                        Lock.unlock();

                                        // Take the handler for request

                                        if (!Handler.Take(
                                                request.Peer,
                                                [this, &request](auto &CB, auto &End)
                                                {
                                                    Format::Serializer Ser(request.Buffer);

                                                    Node node(Identity.Id.Size);
                                                    node.EndPoint = request.Peer;
                                                    Ser >> node.Id;

                                                    CB(node, Ser, End);

                                                    // Cache Test

                                                    // Cache.Add(node);

                                                    Cache.Test(
                                                        node,
                                                        [this](Node Peer, auto OnFail)
                                                        {
                                                            Ping(
                                                                Peer.EndPoint,
                                                                [](Duration duration, Handler::EndCallback End)
                                                                {
                                                                    // if it is successfull dont call end
                                                                },
                                                                [CB = std::move(OnFail)]()
                                                                {
                                                                    Test::Log("Cache") << "Replaced" << std::endl;
                                                                    CB();
                                                                });
                                                        });
                                                }))
                                        {
                                            Respond(request);
                                        }
                                    }))
                            {
                                Lock.unlock();
                            }
                        }
                    }
                    else if (Poll[1].HasEvent())
                    {
                        Test::Log("Handler") << "Request timed out" << std::endl;
                        Handler.Clean(
                            [this](const EndPoint &Target)
                            {
                                Cache.Remove(Target);
                                Test::Log("cache") << "Timed out and removed" << std::endl;
                            });
                        Lock.unlock();
                    }
                    else if (Poll[2].HasEvent())
                    {
                        Interrupt.Listen();
                        goto calc;
                    }
                }
            }

            void Respond(Network::DHT::Request &Request)
            {
                Format::Serializer ReqSerializer(Request.Buffer);
                Node node(Identity.Id.Size);
                char Type;

                node.EndPoint = Request.Peer;
                ReqSerializer >> node.Id;

                // Handle this new node

                Cache.Add(node);

                // Get request type

                ReqSerializer >> Type;

                switch (static_cast<Operations>(Type))
                {
                case Operations::Ping:
                {
                    Fire(
                        Request.Peer,
                        [this](Format::Serializer &Response)
                        {
                            Response << (char)Operations::Response << Identity.Id;
                        });

                    break;
                }
                case Operations::Query:
                {
                    Key key(Identity.Id.Size);

                    ReqSerializer >> key;

                    Fire(
                        Request.Peer,
                        [this, &key](Format::Serializer &Response)
                        {
                            Response << (char)Operations::Response;

                            Response << Cache.Resolve(key);
                        });

                    break;
                }
                case Operations::Set:
                {
                    // Set - Key - Data

                    Key key(Identity.Id.Size);
                    ReqSerializer >> key;

                    Iterable::Span<char> Data = ReqSerializer.Blob();

                    OnSet(key, Data);

                    break;
                }
                case Operations::Get:
                {
                    // Get - Key

                    Key key(Identity.Id.Size);
                    ReqSerializer >> key;

                    OnGet(
                        key,
                        [this, &node](const Iterable::Span<char> &Data)
                        {
                            Fire(
                                node.EndPoint,
                                [&Data](Format::Serializer &Serializer)
                                {
                                    Serializer << (char)Operations::Response << Data;
                                });
                        });

                    break;
                }
                case Operations::Data:
                {
                    Iterable::Span<char> Data = ReqSerializer.Blob();

                    OnData(Data);

                    break;
                }
                case Operations::Invalid:
                {
                    std::string Cause;

                    // OnFail();

                    ReqSerializer >> Cause;

                    break;
                }
                default:
                {
                    Fire(
                        Request.Peer,
                        [this](Format::Serializer &Response)
                        {
                            Response << (char)Operations::Invalid << "Invalid command";
                        });

                    break;
                }
                }
            }

        public:
            // ### Public Variables

            Duration TimeOut;

            // Request Handlers

            typedef std::function<void(Iterable::Span<char> &)> OnGetCallback;

            std::function<void(const Key &, const Iterable::Span<char> &)> OnSet = [](const Core::Network::DHT::Key &Key, const Core::Iterable::Span<char> &Data) {};
            std::function<void(const Key &, OnGetCallback)> OnGet = [](const Core::Network::DHT::Key &Key, OnGetCallback CB) {};
            std::function<void(Iterable::Span<char> &)> OnData = [](Core::Iterable::Span<char> &Data) {};

            // ### Constructors

            Runner() = default;

            Runner(const Node &identity, Duration Timeout, size_t ThreadCount = 1) : Identity(identity), Server(identity.EndPoint), Handler(), Poll(3),
                                                                                     Pool(ThreadCount, false), Cache(Identity.Id), State(States::Stopped), TimeOut(Timeout)
            {
                Poll.Add(Server.Listener(), Server.Events());
                Poll.Add(Handler.Listener(), Timer::In);
                Poll.Add(Interrupt.INode(), Event::In);
            }

            // ### Static functions

            inline static Network::EndPoint DefaultEndPoint()
            {
                return {};
            }

            // ### Functionalities

            // void Await()
            // {
            //     //
            // }

            std::function<void()> GetInPool = [this]()
            {
                EventLoop();

                // try
                // {
                //     EventLoop();
                // }
                // catch (const std::exception &e)
                // {
                //     std::cerr << e.what() << '\n';
                // }

                // Test::Log("Pool") << "Exited" << std::endl;
            };

            void Run()
            {
                // Setup thread pool task

                State = States::Running;

                Pool.Reserve(Pool.Capacity());

                for (size_t i = 0; i < Pool.Capacity(); i++)
                {
                    Pool[i] = std::thread(GetInPool);
                }
            }

            void Stop()
            {
                State = States::Stopped;

                // Join threads in pool

                Pool.ForEach(
                    [](std::thread &Thread)
                    {
                        Thread.join();
                    });
            }

            // Builders for new precedure

            inline void Fire( // @todo Fix with perfect forwarding
                const Core::Network::EndPoint &Peer,
                const std::function<void(Core::Format::Serializer &)> &Builder /*, End*/)
            {
                Server.Put(
                    Peer,
                    [this, &Builder](Format::Serializer &Ser)
                    {
                        // Fill in my id for me

                        Ser << Identity.Id;

                        // Build

                        Builder(Ser);
                    });

                Interrupt.Emit(1);
            }

            bool Build(
                const Core::Network::EndPoint &Peer,
                const std::function<void(Core::Format::Serializer &)> &Builder,
                Core::Network::DHT::Handler::Callback Callback,
                Core::Network::DHT::Handler::EndCallback End)
            {
                if (!Handler.Put(Peer, DateTime::FromNow(TimeOut), std::move(Callback), std::move(End)))
                {
                    End();
                    Test::Error("Handler") << "Handler already exists" << std::endl;
                    return false;
                }

                Fire(Peer, Builder);

                return true;
            }

            // @todo Change callback to refrence to callback

            typedef std::function<void(Duration, Handler::EndCallback)> PingCallback;

            void Ping(const Network::EndPoint &Peer, PingCallback Callback, Handler::EndCallback End)
            {
                Build(
                    Peer,

                    // Build buffer

                    [this](Format::Serializer &Serializer)
                    {
                        Serializer << (char)Operations::Ping << Identity.Id;
                    },

                    // Process response

                    [this, SendTime = DateTime::Now(), CB = std::move(Callback)](Node &key, Format::Serializer &request, Handler::EndCallback End)
                    {
                        // @todo check retuened id as well

                        CB(DateTime::Now() - SendTime, End);
                    },
                    std::move(End));
            }

            typedef std::function<void(Iterable::List<Node>, Handler::EndCallback)> QueryCallback;

            void Query(const Network::EndPoint &Peer, const Key &Id, QueryCallback Callback, Handler::EndCallback End)
            {
                Build(
                    Peer,

                    // Build buffer

                    [&Id](Format::Serializer &Serializer)
                    {
                        Serializer << (char)Operations::Query << Id;
                    },

                    // Process response

                    [this, CB = std::move(Callback)](Node &Requester, Format::Serializer &Serializer, Handler::EndCallback End)
                    {
                        if (Requester.Id == Identity.Id)
                        {
                            End();
                            return;
                        }

                        char Header;

                        Serializer >> Header;

                        try
                        {
                            Iterable::List<Node> node;
                            Serializer >> node;
                            CB(std::move(node), std::move(End));
                        }
                        catch (const std::exception &e)
                        {
                            Test::Warn(e.what()) << Serializer << std::endl;
                            End();
                        }
                    },
                    std::move(End));
            }

            typedef std::function<void(Iterable::List<Node>, Handler::EndCallback)> RouteCallback;

            void Route(const Network::EndPoint &Peer, const Key &Id, RouteCallback Callback, Handler::EndCallback End)
            {
                if (Id == Identity.Id)
                {
                    Callback(Cache.Terminate(), std::move(End));
                    return;
                }

                Query( // @todo optimize functions in the argument
                    Peer,
                    Id,
                    [this, Peer, Id, CB = std::move(Callback)](Iterable::List<Node> Response, const Handler::EndCallback &End)
                    {
                        [[unlikely]] if (Response[0].Id == Identity.Id)
                        {
                            CB(Response, std::move(End));
                            return;
                        }

                        if (Response[0].EndPoint.address() == Network::Address("0.0.0.0")) // <-- @todo Optimize this
                        {
                            Response[0].EndPoint = Peer;
                            CB(std::move(Response), std::move(End));
                            return;
                        }

                        Route(Response[0].EndPoint, Id, std::move(CB), std::move(End));
                    },
                    std::move(End));
            }

            void Route(const Key &Id, RouteCallback Callback, Handler::EndCallback End)
            {
                const auto &Peer = Cache.Resolve(Id);

                [[unlikely]] if (Peer[0].Id == Identity.Id)
                {
                    Callback(Cache.Terminate(), std::move(End));
                    return;
                }

                Route(Peer[0].EndPoint, Id, std::move(Callback), std::move(End));
            }

            typedef std::function<void(Handler::EndCallback)> BootstrapCallback;

            void Bootstrap(const Network::EndPoint &Peer, size_t NthNeighbor, BootstrapCallback Callback, Handler::EndCallback End)
            {
                // @todo Add the bootstrap node after asked for its id

                auto Neighbor = Identity.Id.Neighbor(NthNeighbor);

                Route(
                    Peer,
                    Neighbor,
                    [this, NthNeighbor, CB = std::move(Callback)](Iterable::List<Node> Response, Handler::EndCallback End)
                    {
                        if (Response[0].Id == Identity.Id)
                        {
                            // All nodes resode before my key

                            End();
                            return;
                        }

                        // @todo also not all nodes exist and need to be routed

                        size_t i = Cache.NeighborHood(Response[0].Id);

                        if (i > NthNeighbor)
                        {
                            // Wrapped around and bootstrap is done

                            End();
                            return;
                        }

                        if (i > 0)
                        {
                            Bootstrap(Cache.Resolve(i)[0].EndPoint, i, std::move(CB), std::move(End));
                        }
                        else
                        {
                            End();
                        }
                    },
                    std::move(End)); // @todo do a self look up in the end
            }

            inline void Bootstrap(const Network::EndPoint &Peer, BootstrapCallback Callback, Handler::EndCallback End)
            {
                Bootstrap(Peer, Identity.Id.Size, std::move(Callback), std::move(End));
            }

            typedef std::function<void(Iterable::Span<char> &Data, Handler::EndCallback)> GetCallback;

            void GetFrom(const Network::EndPoint &Peer, const Key &key, GetCallback Callback, Handler::EndCallback End)
            {
                Build(
                    Peer,

                    // Build buffer
                    // Get - Key

                    [&key](Format::Serializer &Serializer)
                    {
                        Serializer << (char)Operations::Get << key;
                    },
                    [this, CB = std::move(Callback)](Node &Requester, Format::Serializer &Serializer, Handler::EndCallback End)
                    {
                        char Header;

                        Serializer >> Header;

                        Iterable::Span<char> Data(Serializer.Length());

                        Serializer >> Data;

                        CB(Data, End);
                    },
                    std::move(End));
            }

            void Get(const Key &key, GetCallback Callback, Handler::EndCallback End)
            {
                Route(
                    key,
                    [this, key, CB = std::move(Callback)](Iterable::List<Node> Peers, Handler::EndCallback End)
                    {
                        if (Peers[0].Id == Identity.Id)
                        {
                            OnGet(
                                key,
                                [CB = std::move(CB), End = std::move(End)](Iterable::Span<char> &Data)
                                {
                                    CB(Data, End);
                                });

                            return;
                        }

                        GetFrom(Peers[0].EndPoint, key, CB, std::move(End));
                    },
                    std::move(End));
            }

            // @todo Maybe add send with call back too?

            void SetTo(const Network::EndPoint &Peer, const Key &key, const Iterable::Span<char> &Data)
            {
                Fire(
                    Peer,
                    [&key, &Data](Format::Serializer &Serializer)
                    {
                        Serializer << (char)Operations::Set << key << Data;
                    });
            }

            void Set(const Key &key, const Iterable::Span<char> &Data, Handler::EndCallback End)
            {
                Route(
                    key,
                    [this, key, Data](Iterable::List<Node> Peers, Handler::EndCallback End)
                    {
                        if (Peers[0].Id == Identity.Id)
                        {
                            OnSet(key, Data);
                            End(); // <-- Fix this
                            return;
                        }

                        SetTo(Peers[0].EndPoint, key, Data);

                        End();
                    },
                    std::move(End));
            }

            // Send data

            void SendTo(const Network::EndPoint &Peer, const Iterable::Span<char> &Data)
            {
                Fire(
                    Peer,
                    [&Data](Format::Serializer &Serializer)
                    {
                        Serializer << (char)Operations::Data << Data;
                    });
            }

            void SendWhere(const Network::EndPoint &Peer, const Key &key, const Iterable::Span<char> &Data,
                           const std::function<bool(const Node &)> &Condition)
            {
                Cache.ForEach(
                    [this, &Data, &Condition](const Node &node)
                    {
                        if (Condition(node))
                            Fire(
                                node.EndPoint,
                                [this, &Data](Format::Serializer &Serializer)
                                {
                                    Serializer << (char)Operations::Data << Data;
                                });
                    });
            }

            // // Flood

            void Flood(const Network::EndPoint &Peer, const Key &key, const Iterable::Span<char> &Data)
            {
                Cache.ForEach(
                    [this, &Data](const Node &Node)
                    {
                        Fire(
                            Node.EndPoint,
                            [this, &Data](Format::Serializer &Serializer)
                            {
                                Serializer << (char)Operations::Data << Data;
                            });
                    });
            }
        };
    }
}