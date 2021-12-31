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

                private:
                    // ### Types

                    // ### Variables

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
                            Handler::Callback Task;
                            Handler::EndCallback End;

                            Poll.Await();

                            if (Poll[0].HasEvent())
                            {
                                if (Server.State != Network::DHT::Server::States::Running)
                                {
                                    break;
                                }

                                if (!Server.Take(Request)) // @todo Clarify this
                                    continue;

                                if (Handler.Take(Request.Peer, Task, End))
                                    Task(Request, End);
                                else
                                    _Default(Request);
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

                    void _Default(Network::DHT::Request &Request)
                    {
                        Network::DHT::Request Response(Request.Peer, 1024);
                        
                        Conversion::Serializer ReqSerializer(Request.Buffer);
                        Conversion::Serializer ResSerializer(Response.Buffer);

                        char Type;

                        ReqSerializer >> Type;

                        switch (static_cast<Operations>(Type))
                        {
                        case Operations::Ping:
                            Test::Log("Ping") << Request.Peer << std::endl;
                            ResSerializer << (char)Operations::Response << Id;
                            break;
                        case Operations::Query:
                            Test::Log("Query") << Request.Peer << std::endl;
                            break;
                        case Operations::Set:
                            Test::Log("Set") << Request.Peer << std::endl;
                            break;
                        case Operations::Get:
                            Test::Log("Get") << Request.Peer << std::endl;
                            break;
                        case Operations::Data:
                            Test::Log("Data") << Request.Peer << std::endl;
                            break;
                        case Operations::Deliver:
                            Test::Log("Deliver") << Request.Peer << std::endl;
                            break;
                        case Operations::Flood:
                            Test::Log("Flood") << Request.Peer << std::endl;
                            break;
                        default:
                            ResSerializer << (char)Operations::Response << "Unknown command";
                            Test::Log("Unknown") << Request.Peer << " : " << Request.Buffer << std::endl;
                            break;
                        }

                        Server.Put(std::move(Response));
                    };

                public:
                    // ### Public Variables

                    time_t TimeOut;

                    // ### Constructors

                    Runner() = default;

                    Runner(Key id, const Network::EndPoint &EndPoint, size_t ThreadCount = 1, time_t Timeout = 60) : Id(id), Server(EndPoint),
                                                                                                                     Handler(), Pool(ThreadCount, false), TimeOut(Timeout) {}

                    // ### Static functions

                    inline Network::EndPoint DefaultEndPoint()
                    {
                        return Network::EndPoint("127.0.0.1:8888");
                    }

                    // ### Functionalities

                    void Add(Node &&node) // <- @todo Fix with perfect forwarding
                    {
                        // Nodes.AddSorted(std::move(node)); // <- Add compare operators for Node
                        Nodes.Add(std::forward<Node>(node));
                    }

                    void Add(const Node &node)
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
                        size_t Count = Nodes.Length();

                        if (Count == 1)
                            return Nodes[0];

                        for (Index = 0; Index < Nodes.Length() && Nodes[Index].Id < key; Index++)
                        {
                        }

                        return Nodes[Index - 1];
                    }

                    // @todo Add on fail callback and bool return value for functions
                    // @todo Change callback to refrence to callback

                    typedef std::function<void(Network::EndPoint, Handler::EndCallback)> QueryCallback;

                    void Query(Network::EndPoint Peer, Key Id, QueryCallback Callback, Handler::EndCallback End)
                    {
                        // Add Handler

                        auto Result = Handler.Put( // @todo <---- this can initialize or generate a buffer containing header
                            Peer,
                            DateTime::FromNow(TimeOut),
                            [this, CB = std::move(Callback)](Network::DHT::Request &request, Handler::EndCallback End)
                            {
                                auto Response = request.Buffer.ToString();

                                try
                                {
                                    CB(Response, End);
                                }
                                catch (const std::exception &e)
                                {
                                    Test::Warn(e.what()) << Response << std::endl;
                                    End();
                                }
                            },
                            End);

                        if (!Result)
                        {
                            End();
                            Test::Error("Ping") << "Handler already exists" << std::endl;
                            return;
                        }

                        Server.Put(
                            Peer,
                            [&Id](Conversion::Serializer& Serializer)
                            {
                                Serializer << (char)Operations::Query << Id;
                            },
                            Id.Size);
                    }

                    typedef std::function<void(Network::EndPoint, Handler::EndCallback)> RouteCallback;

                    void Route(Network::EndPoint Peer, Key Id, RouteCallback Callback, const Handler::EndCallback &End)
                    {
                        Query( // @todo optimize functions in the argument
                            Peer,
                            Id,
                            [this, Peer, Id, CB = std::move(Callback)](Network::EndPoint Response, const Handler::EndCallback &End)
                            {
                                if (Peer == Response)
                                    CB(Peer, End);
                                else
                                    Route(Response, Id, CB, End);
                            },
                            End);
                    }

                    void Route(const Key &Id, RouteCallback Callback, Handler::EndCallback End)
                    {
                        const auto &Peer = Closest(Id).EndPoint;

                        Route(Peer, Id, std::move(Callback), std::move(End));
                    }

                    typedef std::function<void(Handler::EndCallback)> BootstrapCallback;

                    void Bootstrap(Network::EndPoint Peer, size_t NthNeighbor, BootstrapCallback Callback, Handler::EndCallback End)
                    {
                        auto Neighbor = Id.Neighbor(NthNeighbor);

                        Route(
                            Peer,
                            Neighbor,
                            [this, NthNeighbor, CB = std::move(Callback)](Network::EndPoint Response, Handler::EndCallback End)
                            {
                                Nodes.Add(Node(Id, Response));

                                // @todo also not all nodes exist and need to be routed

                                Bootstrap(Closest(Id).EndPoint, NthNeighbor - 1, CB, End);
                            },
                            End);
                    }

                    void Bootstrap(Network::EndPoint Peer, BootstrapCallback Callback, Handler::EndCallback End)
                    {
                        Bootstrap(Closest(Id).EndPoint, Id.Size, std::move(Callback), std::move(End));
                    }

                    typedef std::function<void(Duration, Handler::EndCallback)> PingCallback;

                    void Ping(Network::EndPoint Peer, PingCallback Callback, Handler::EndCallback End)
                    {
                        auto SendTime = DateTime::Now();

                        auto Result = Handler.Put(
                            Peer,
                            DateTime::FromNow(TimeOut),
                            [this, SendTime, CB = std::move(Callback)](Network::DHT::Request &request, Handler::EndCallback End)
                            {
                                // @todo check retuened id as well

                                CB(DateTime::Now() - SendTime, End);
                            },
                            End);

                        if (!Result)
                        {
                            End();
                            Test::Error("Ping") << "Handler already exists" << std::endl;
                            return;
                        }

                        Server.Put(
                            Peer,
                            [this](Conversion::Serializer& Serializer)
                            {
                                Serializer << (char)Operations::Ping << Id;
                            },
                            Id.Size);
                    }

                    typedef std::function<void(Iterable::Queue<char> &Data, Handler::EndCallback)> GetCallback;

                    void Get(Network::EndPoint Peer, Key key, GetCallback Callback, Handler::EndCallback End)
                    {
                        auto Result = Handler.Put(
                            Peer,
                            DateTime::FromNow(TimeOut),
                            [this, CB = std::move(Callback)](Network::DHT::Request &request, Handler::EndCallback End)
                            {
                                // @todo check retuened id as well

                                CB(request.Buffer, End);
                            },
                            End);

                        if (!Result)
                        {
                            End();
                            Test::Error("Ping") << "Handler already exists" << std::endl;
                            return;
                        }

                        Server.Put(
                            Peer,
                            [&key](Conversion::Serializer& Serializer)
                            {
                                Serializer << (char)Operations::Get << key;
                            },
                            key.Size);
                    }

                    void Get(Key key, GetCallback Callback, Handler::EndCallback End)
                    {
                        Route(
                            key,
                            [this, key, CB = std::move(Callback)](auto Target, Handler::EndCallback End)
                            {
                                Get(Target, key, CB, End);
                            },
                            End);
                    }

                    typedef std::function<void(Handler::EndCallback)> SetCallback;

                    void Set(Network::EndPoint Peer, Key key, const Iterable::Span<char> &Data, SetCallback Callback, Handler::EndCallback End)
                    {
                        auto Result = Handler.Put(
                            Peer,
                            DateTime::FromNow(TimeOut),
                            [this, CB = std::move(Callback)](Network::DHT::Request &request, Handler::EndCallback End)
                            {
                                CB(End);
                            },
                            End);

                        if (!Result)
                        {
                            End();
                            Test::Error("Ping") << "Handler already exists" << std::endl;
                            return;
                        }

                        Server.Put(
                            Peer,
                            [&key, &Data](Conversion::Serializer& Serializer)
                            {
                                Serializer << (char)Operations::Set << key << Data;
                            },
                            key.Size);
                    }

                    void Set(Key key, const Iterable::Span<char> &Data, SetCallback Callback, Handler::EndCallback End)
                    {
                        Route(
                            key,
                            [this, key, Data, CB = std::move(Callback)](auto Target, Handler::EndCallback End)
                            {
                                Set(Target, key, Data, CB, End);
                            },
                            End);
                    }
                };
            }
        }
    }
}