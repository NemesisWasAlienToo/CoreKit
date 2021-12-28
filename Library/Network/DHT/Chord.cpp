#pragma once

#include <iostream>
#include <string>
#include <thread>

#include "Network/EndPoint.cpp"
#include "Network/DHT/Key.cpp"
#include "Network/DHT/Server.cpp"
#include "Network/DHT/Handler.cpp"
#include "Network/DHT/Node.cpp"
#include "Iterable/List.cpp"
#include "Iterable/Poll.cpp"
#include "Conversion/Serializer.cpp"
#include "Test.cpp"
#include "Duration.cpp"

using namespace Core;

namespace Core
{
    namespace Network
    {
        namespace DHT
        {
            namespace Chord
            {
                class Runner
                {
                public:
                    typedef std::function<void()> DoneCallBack;

                private:
                    // ### Types

                    // ### Variables

                    enum class Operations : char
                    {
                        Response = 0,
                        Ping,
                        Query,
                        Set,
                        Get,
                        Data,
                        Deliver,
                        Flood,
                    };

                    enum class Events : char
                    {
                        Incomming = 0,
                        Exit,
                        TimeOut,
                    };

                    Key Id;

                    Network::DHT::Server Server;

                    Network::DHT::Handler Handler;

                    Iterable::List<std::thread> Pool;

                    Iterable::List<Node> Nodes;

                    std::function<void()> _PoolTask =
                        [this]()
                    {
                        // Clear running event

                        Iterable::Poll Poll(2);

                        Poll.Add(Server.Listener(), Descriptor::In);
                        Poll.Add(Handler.Listener(), Descriptor::In);

                        while (Server.State == Network::DHT::Server::States::Running)
                        {
                            Network::DHT::Request Request;
                            std::function<void(Network::DHT::Request &)> Task;

                            Poll.Await();

                            if (Poll[0].HasEvent())
                            {
                                if (Server.State != Network::DHT::Server::States::Running)
                                {
                                    break;
                                }

                                if (!Server.Take(Request))
                                    continue;

                                Handler.Take(Request.Peer, Task);

                                Task(Request);
                            }

                            if (Poll[1].HasEvent())
                            {
                                Test::Warn("Timeout") << "Request timed out" << std::endl;
                                Handler.Clean();
                            }
                        }

                        Server.Ready.Emit(1);

                        std::cout << "Thread exited" << std::endl;
                    };

                    std::function<void(Network::DHT::Request &)> _Default =
                        [this](Network::DHT::Request &Request)
                    {
                        Test::Log(Request.Peer.ToString()) << Request.Buffer << std::endl;
                    };

                public:
                    // ### Public Variables

                    time_t TimeOut;

                    // ### Constructors

                    Runner() = default;

                    Runner(Key id, const Network::EndPoint &EndPoint, size_t ThreadCount = 1, time_t Timeout = 60) : Id(id), Server(EndPoint),
                                                                                                                     Handler(_Default), Pool(ThreadCount, false), TimeOut(Timeout) {}

                    // ### Static functions

                    inline Network::EndPoint DefaultEndPoint()
                    {
                        return Network::EndPoint("127.0.0.1:8888");
                    }

                    // ### Functionalities

                    void AddNode(Node &&node) // <- @todo Fix with perfect forwarding
                    {
                        // Nodes.AddSorted(std::move(node)); // <- Add compare operators for Node
                        Nodes.Add(std::move(node));
                    }

                    void AddNode(const Node &node)
                    {
                        // Nodes.AddSorted(node); // <- Add compare operators for Node
                        Nodes.Add(node);
                    }

                    void Await()
                    {
                        // Wait for all out tasks to be done
                    }

                    inline void GetInPool()
                    {
                        _PoolTask();
                    }

                    void Run()
                    {
                        // Start the udp serializer server

                        Server.Run();

                        // Setup thread pool task

                        Pool.Reserver(Pool.Capacity());

                        for (size_t i = 0; i < Pool.Capacity(); i++)
                        {
                            Pool[i] = std::thread(_PoolTask);
                        }
                    }

                    void Stop()
                    {
                        // Stop the server

                        Server.Stop();

                        // Join threads

                        Pool.ForEach(
                            [](std::thread &Thread)
                            {
                                Thread.join();
                            });

                        // Clear event

                        Server.Ready.Listen();
                    }

                    // Network requests

                    Node &Closest(const Key &key)
                    {
                        if (Nodes.Length() <= 0)
                            throw std::out_of_range("Instance contains no node data");

                        size_t Index;

                        for (Index = 0; Index < Nodes.Length() && Nodes[Index].Id < key; Index++)
                        {
                        }

                        return Nodes[Index - 1];
                    }

                    // @todo Add on fail callback and bool return value for functions
                    // @todo Change callback to refrence to callback

                    void Query(Network::EndPoint Peer, Key Id, std::function<void(Network::EndPoint)> Callback)
                    {
                        // Add Handler

                        auto Result = Handler.Put(
                            Peer,
                            DateTime::FromNow(TimeOut),
                            [this, CB = std::move(Callback)](Network::DHT::Request &request)
                            {
                                auto Response = request.Buffer.ToString();

                                try
                                {
                                    CB(Response);
                                }
                                catch (const std::exception &e)
                                {
                                    Test::Warn(e.what()) << Response << std::endl;
                                }
                            });

                        if (!Result)
                        {
                            Test::Error("Ping") << "Handler already exists" << std::endl;
                            return;
                        }

                        Network::DHT::Request req(Peer, Id.Size + 1);

                        Network::Serializer Serializer(req.Buffer); // <-- @todo Add CHRD header here

                        // Form request

                        Serializer << static_cast<char>(Operations::Query);

                        Serializer << Id;

                        Server.Put(std::move(req));

                        // return true;
                    }

                    void Route(Network::EndPoint Peer, Key Id, std::function<void(Network::EndPoint)> Callback)
                    {
                        Query(
                            Peer,
                            Id,
                            [this, Peer, Id, CB = std::move(Callback)](Network::EndPoint Response)
                            {
                                if (Peer == Response)
                                    CB(Peer);
                                else
                                    Route(Response, Id, CB);
                            });
                    }

                    void Route(const Key& Id, std::function<void(Network::EndPoint)> Callback)
                    {
                        const auto &Peer = Closest(Id).EndPoint;

                        // Route(Peer, Id, std::move(Callback));
                        Route(Peer, Id, Callback);
                    }

                    void Bootstrap(Network::EndPoint Peer, size_t NthNeighbor, std::function<void()> Callback)
                    {
                        auto Neighbor = Id.Neighbor(NthNeighbor);

                        Route(
                            Peer,
                            Neighbor,
                            [this, NthNeighbor, CB = std::move(Callback)](Network::EndPoint Response)
                            {
                                Nodes.Add(Node(Id, Response));

                                // @todo also not all nodes exist and need to be routed

                                Bootstrap(Closest(Id).EndPoint, NthNeighbor - 1, CB);
                            });
                    }

                    void Bootstrap(Network::EndPoint Peer, std::function<void()> Callback)
                    {
                        Bootstrap(Closest(Id).EndPoint, Id.Size, std::move(Callback));
                    }

                    void Ping(Network::EndPoint Peer, std::function<void(Duration)> Callback /*, std::function<void(size_t)> OnFail*/)
                    {
                        auto SendTime = DateTime::Now();

                        auto Result = Handler.Put(
                            Peer,
                            DateTime::FromNow(TimeOut),
                            [this, SendTime, CB = std::move(Callback)](Network::DHT::Request &request)
                            {
                                // @todo check retuened id as well

                                CB(DateTime::Now() - SendTime);
                            });

                        if (!Result)
                        {
                            Test::Error("Ping") << "Handler already exists" << std::endl;
                            return;
                        }

                        // @todo Add CHRD tag here?

                        Network::DHT::Request req(Peer, (Id.Size * 2) + 1); // <-- Check buffer init size

                        Network::Serializer Serializer(req.Buffer); // <-- @todo Add CHRD header here

                        // Form request

                        Serializer << static_cast<char>(Operations::Ping);

                        Serializer << Id.ToString();

                        Server.Put(std::move(req));
                    }

                    void Get(Network::EndPoint Target, Key key, std::function<void(Iterable::Queue<char> &Data)> Callback)
                    {
                        auto Result = Handler.Put(
                            Target,
                            DateTime::FromNow(TimeOut),
                            [this, CB = std::move(Callback)](Network::DHT::Request &request)
                            {
                                // @todo check retuened id as well

                                CB(request.Buffer);
                            });

                        if (!Result)
                        {
                            Test::Error("Ping") << "Handler already exists" << std::endl;
                            return;
                        }

                        // @todo Add CHRD tag here?

                        Network::DHT::Request req(Target, (Id.Size * 2) + 1);

                        Network::Serializer Serializer(req.Buffer); // <-- @todo Add CHRD header here

                        // Form request

                        Serializer << static_cast<char>(Operations::Get);

                        Serializer << Id.ToString();

                        Server.Put(std::move(req));
                    }

                    void Get(Key key, std::function<void(Iterable::Queue<char> &Data)> Callback)
                    {
                        Route(
                            key,
                            [this, key, CB = std::move(Callback)](auto Target)
                            {
                                Get(Target, key, CB);
                            });
                    }

                    void Set(Network::EndPoint Target, Key key, const Iterable::Span<char> &Data, std::function<void()> Callback)
                    {
                        auto Result = Handler.Put(
                            Target,
                            DateTime::FromNow(TimeOut),
                            [this, CB = std::move(Callback)](Network::DHT::Request &request)
                            {
                                CB();
                            });

                        if (!Result)
                        {
                            Test::Error("Ping") << "Handler already exists" << std::endl;
                            return;
                        }

                        // @todo Add CHRD tag here?

                        Network::DHT::Request req(Target, (Id.Size * 2) + 1);

                        Network::Serializer Serializer(req.Buffer); // <-- @todo Add CHRD header here

                        // Form request

                        Serializer << static_cast<char>(Operations::Set) << Id.ToString() << Data;

                        Server.Put(std::move(req));
                    }

                    void Set(Key key, Iterable::Span<char> &Data, std::function<void()> Callback)
                    {
                        Route(
                            key,
                            [this, key, Data, CB = std::move(Callback)](auto Target)
                            {
                                Set(Target, key, Data, CB);
                            });
                    }
                };
            }
        }
    }
}