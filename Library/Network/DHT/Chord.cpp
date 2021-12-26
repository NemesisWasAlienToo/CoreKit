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
#include "Test.cpp"

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
                private:
                    // ### Types

                    // ### Variables

                    enum class Operations : unsigned char
                    {
                        Ping = 0,
                        Query,
                        Set,
                        Get,
                        Data,
                        Deliver,
                        Flood,
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

                        while (Server.State == Network::DHT::Server::States::Running)
                        {
                            Network::DHT::Request Request;
                            std::function<void(Network::DHT::Request &)> Task;

                            // Pick one request

                            if (!Server.Take(Request))
                                continue;

                            // Pick one task or default task

                            Handler.Take(Request.Peer, Task);

                            // Execute task

                            Task(Request);
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
                    // ### Constructors

                    Runner() = default;

                    Runner(Key id, const Network::EndPoint &EndPoint, size_t ThreadCount = 1) : Id(id), Server(EndPoint),
                                                                                                Handler(_Default), Pool(ThreadCount, false) {}

                    // ### Static functions

                    Network::EndPoint DefaultEndPoint()
                    {
                        return Network::EndPoint("127.0.0.1:8888");
                    }

                    // ### Functionalities

                    void Await()
                    {
                        // Wait for out tasks to be done
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

                        Pool.Add(std::thread(_PoolTask), Pool.Capacity());
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

                    Node &Closest(Key key)
                    {
                        size_t Index;

                        for (size_t i = 0; i < Nodes.Length() && Nodes[i].Id < key; i++) {}

                        return Nodes[Index - 1];
                    }

                    void Query(Network::EndPoint Peer, Key Id, std::function<void(Network::EndPoint)> Callback)
                    {
                        // @todo set handler before write to detect in progress request
                        // Write packet

                        Network::DHT::Request req;

                        std::string _Id = Id.ToString();

                        req.Peer = Peer;
                        req.Buffer.Add(_Id.c_str(), _Id.length());

                        Server.Put(std::move(req));

                        // Add Handler

                        auto Result = Handler.Put(
                            Peer,
                            DateTime::FromNow(/* Timeout */),
                            [this, Callback](Network::DHT::Request &request)
                            {
                                auto Response = request.Buffer.ToString();

                                try
                                {
                                    Callback(Response);
                                }
                                catch (const std::exception &e)
                                {
                                    Test::Warn(e.what()) << Response << std::endl;
                                }
                            });

                        if (!Result)
                            std::cout << "Handler already exists" << std::endl;
                    }

                    void Route(Network::EndPoint Peer, Key Id, std::function<void(Network::EndPoint)> Callback)
                    {
                        Query(
                            Peer,
                            Id,
                            [this, Peer, Id, Callback](Network::EndPoint Response)
                            {
                                if (Peer == Response)
                                    Callback(Peer);
                                else
                                    Route(Response, Id, Callback);
                            });
                    }

                    void Route(Key Id, std::function<void(Network::EndPoint)> Callback)
                    {
                        const auto &Peer = Closest(Id).EndPoint;

                        Route(
                            Peer,
                            Id,
                            [this, Peer, Id, Callback](Network::EndPoint Response)
                            {
                                if (Peer == Response)
                                    Callback(Peer);
                                else
                                    Route(Response, Id, Callback);
                            });
                    }

                    void Bootstrap(Network::EndPoint Peer, size_t NthNeighbor)
                    {
                        auto Neighbor = Id.Neighbor(NthNeighbor);

                        Route(
                            Peer,
                            Neighbor,
                            [this, NthNeighbor](Network::EndPoint Response)
                            {
                                Nodes.Add(Node(Id, Response));

                                // @todo also not all nodes exist and need to be routed

                                Bootstrap(Closest(Id).EndPoint, NthNeighbor - 1);
                            });
                    }

                    void Bootstrap()
                    {
                        Bootstrap(Closest(Id).EndPoint, Id.Size);
                    }

                    // int Ping(Network::EndPoint Target, int TimeOut)
                    // {
                    //     //
                    // }

                    // Network::EndPoint Get(Network::EndPoint Target)
                    // {
                    //     //
                    // }

                    // Network::EndPoint Set(Network::EndPoint Target)
                    // {
                    //     //
                    // }
                };
            }
        }
    }
}