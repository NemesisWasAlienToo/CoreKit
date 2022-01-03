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
#include "Format/Serializer.cpp"
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

                    Node Identity;

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
                                    Respond(Request);
                            }

                            if (Poll[1].HasEvent())
                            {
                                Test::Warn("Timeout") << "Request timed out" << std::endl;
                                Handler.Clean();
                                // OnTimeOut()
                            }
                        }

                        Server.Ready.Emit(1);

                        std::cout << "Thread exited" << std::endl;
                    };

                    void OnTimeOut(const Network::EndPoint &Peer)
                    {
                        //
                    }

                    void Respond(Network::DHT::Request &Request)
                    {
                        Format::Serializer ReqSerializer(Request.Buffer);

                        char Type;

                        ReqSerializer >> Type;

                        switch (static_cast<Operations>(Type))
                        {
                        case Operations::Ping:
                        {
                            Test::Log("Ping") << Request.Peer << std::endl;

                            Server.Put(
                                Request.Peer,
                                [this](Format::Serializer &Response)
                                {
                                    Response << (char)Operations::Response << Identity.Id;
                                });

                            break;
                        }
                        case Operations::Query:
                        {
                            Key key(32);

                            ReqSerializer >> key;

                            Server.Put(
                                Request.Peer,
                                [this, &key](Format::Serializer &Response)
                                {
                                    Response << (char)Operations::Response;

                                    size_t Index;

                                    if (Nodes.ContainsWhere(
                                            Index,
                                            [&key](Node &Item) -> bool
                                            {
                                                return Item.Id < key;
                                            }))
                                    {
                                        Response << Nodes[Index].EndPoint;
                                    }
                                    else
                                    {
                                        Response << Identity.Id;
                                    }
                                });

                            Test::Log("Query") << Request.Peer << std::endl;

                            break;
                        }
                        case Operations::Set:
                        {
                            //  @todo Handle
                            Test::Log("Set") << Request.Peer << std::endl;
                            break;
                        }
                        case Operations::Get:
                        {
                            //  @todo Handle
                            Test::Log("Get") << Request.Peer << std::endl;
                            break;
                        }
                        case Operations::Data:
                        {
                            //  @todo Handle
                            Test::Log("Data") << Request.Peer << std::endl;
                            break;
                        }
                        case Operations::Deliver:
                        {
                            Key key(32);

                            ReqSerializer >> key;

                            size_t Index;

                            // @todo can be optimized by only reading and forwarding the same buffer?

                            if (Nodes.ContainsWhere(
                                    Index,
                                    [&key](Node &Item) -> bool
                                    {
                                        return Item.Id < key;
                                    }))
                            {
                                Server.Put(
                                    Nodes[Index].EndPoint,
                                    [this, &key, &ReqSerializer](Format::Serializer &Response)
                                    {
                                        Response << (char)Operations::Deliver << key << ReqSerializer;
                                    });
                            }
                            else
                            {
                                // Act as Data
                            }

                            Test::Log("Deliver") << Request.Peer << std::endl;

                            break;
                        }
                        case Operations::Flood:
                        {
                            // @todo add hod many steps should this be passed
                            uint16_t Count;

                            ReqSerializer >> Count;

                            Iterable::Span<char> Data(ReqSerializer.Length());

                            ReqSerializer >> Data;

                            // Handle data

                            if (Count > 0)
                            {
                                Count--;

                                Nodes.ForEach(
                                    [this, &Count, &Data](Node &Item)
                                    {
                                        Server.Put(
                                            Item.EndPoint,
                                            [&Count, &Data](Format::Serializer &Serializer)
                                            {
                                                Serializer << (char)Operations::Flood << Count << Data;
                                            });
                                    });
                            }

                            Test::Log("Flood") << Request.Peer << std::endl;
                            break;
                        }
                        default:
                        {
                            Server.Put(
                                Request.Peer,
                                [this](Format::Serializer &Response)
                                {
                                    Response << (char)Operations::Response << "Unknown command";
                                });

                            Test::Log("Unknown") << Request.Peer << " : " << Request.Buffer << std::endl;
                            break;
                        }
                        }
                    };

                public:
                    // ### Public Variables

                    time_t TimeOut;

                    // ### Constructors

                    Runner() = default;

                    Runner(const Node &identity, size_t ThreadCount = 1, time_t Timeout = 60) : Identity(identity), Server(identity.EndPoint), Handler(), Pool(ThreadCount, false), TimeOut(Timeout)
                    {
                        Nodes.Add({Identity.Id, {"0.0.0.0:0"}});
                    }

                    // ### Static functions

                    inline static Network::EndPoint DefaultEndPoint()
                    {
                        return Network::EndPoint("127.0.0.1:8888");
                    }

                    // ### Functionalities

                    Node Resolve(Key key)
                    {
                        size_t Index;

                        if (Nodes.ContainsWhere(
                                Index,
                                [this, &key](Node &Item) -> bool
                                {
                                    return Item.Id > key;
                                }))
                        {
                            return Nodes[Index];
                        }
                        else
                        {
                            return Nodes.Last();
                        }
                    }

                    void Add(Node &&node) // <- @todo Fix with perfect forwarding
                    {
                        size_t Index;

                        if (Nodes.ContainsWhere(
                                Index,
                                [this, &node](Node &Item) -> bool
                                {
                                    return Item.Id > node.Id;
                                }))
                        {
                            Nodes.Squeeze(std::forward<Node>(node), Index);
                        }
                        else
                        {
                            Nodes.Add(std::forward<Node>(node));
                        }
                    }

                    void Add(const Node &node)
                    {
                        size_t Index;

                        if (Nodes.ContainsWhere(
                                Index,
                                [this, &node](Node &Item) -> bool
                                {
                                    return Item.Id > node.Id;
                                }))
                        {
                            Nodes.Squeeze(node, Index);
                        }
                        else
                        {
                            Nodes.Add(node);
                        }
                    }

                    void Await()
                    {
                        // @todo Wait for all out tasks to be done IMPORTANT
                    }

                    inline void GetInPool()
                    {
                        _PoolTask();
                    }

                    void Run()
                    {
                        // Start the udp server

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

                        // Join threads in pool

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

                    bool Build(
                        const Core::Network::EndPoint &Peer,
                        const std::function<void(Core::Format::Serializer &)> &Builder,
                        Core::Network::DHT::Handler::Callback Callback,
                        Core::Network::DHT::Handler::EndCallback End)
                    {
                        if (!Handler.Put(Peer, DateTime::FromNow(TimeOut), std::move(Callback), std::move(End)))
                        {
                            End();
                            Test::Error("Chord") << "Handler already exists" << std::endl;
                            return false;
                        }

                        Server.Put(Peer, Builder);

                        return true;
                    }

                    void Fire(
                        const Core::Network::EndPoint &Peer,
                        const std::function<void(Core::Format::Serializer &)> &Builder/*, End*/)
                    {
                        Server.Put(Peer, Builder);
                    }

                    // @todo Change callback to refrence to callback

                    typedef std::function<void(Duration, Handler::EndCallback)> PingCallback;

                    void Ping(const Network::EndPoint& Peer, PingCallback Callback, Handler::EndCallback End)
                    {
                        Build(
                            Peer,

                            // Build buffer

                            // Operation

                            [this](Format::Serializer &Serializer)
                            {
                                Serializer << (char)Operations::Ping << Identity.Id;
                            },

                            // Process response

                            [this, SendTime = DateTime::Now(), CB = std::move(Callback)](Network::DHT::Request &request, Handler::EndCallback End)
                            {
                                // @todo check retuened id as well

                                CB(DateTime::Now() - SendTime, End);
                                End();
                            },
                            End);
                    }

                    typedef std::function<void(Node, Handler::EndCallback)> QueryCallback;

                    void Query(const Network::EndPoint& Peer, const Key& Id, QueryCallback Callback, Handler::EndCallback End)
                    {
                        Build(
                            Peer,

                            // Build buffer

                            // Operation - Key

                            [&Id](Format::Serializer &Serializer)
                            {
                                Serializer << (char)Operations::Query << Id;
                            },

                            // Process response

                            // Response - Node

                            [this, CB = std::move(Callback)](Network::DHT::Request &request, Handler::EndCallback End)
                            {
                                Format::Serializer Serializer(request.Buffer); // <-- maybe already pass this?

                                char Header;
                                Node node;

                                Serializer >> Header;

                                try
                                {
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

                    typedef std::function<void(Node, Handler::EndCallback)> RouteCallback;

                    void Route(const Network::EndPoint& Peer, const Key& Id, RouteCallback Callback, Handler::EndCallback End)
                    {
                        Query( // @todo optimize functions in the argument
                            Peer,
                            Id,
                            [this, Peer, Id, CB = std::move(Callback)](Node Response, const Handler::EndCallback &End)
                            {
                                if (Response.EndPoint == Network::EndPoint("0.0.0.0:0")) // <-- @todo Optimize this
                                {
                                    Response.EndPoint = Peer;
                                    CB(std::move(Response), std::move(End));
                                }
                                else
                                    Route(Response.EndPoint, Id, std::move(CB), std::move(End));
                            },
                            std::move(End));
                    }

                    void Route(const Key &Id, RouteCallback Callback, Handler::EndCallback End)
                    {
                        if(Nodes.Length() <= 1) throw std::underflow_error("No other node is known");

                        const auto &Peer = Closest(Id).EndPoint;

                        Route(Peer, Id, std::move(Callback), std::move(End));
                    }

                    typedef std::function<void(Handler::EndCallback)> BootstrapCallback;

                    void Bootstrap(const Network::EndPoint& Peer, size_t NthNeighbor, BootstrapCallback Callback, Handler::EndCallback End)
                    {
                        // @todo Add the bootstrap node after asked for its id

                        auto Neighbor = Identity.Id.Neighbor(NthNeighbor);

                        Route(
                            Peer,
                            Neighbor,
                            [this, NthNeighbor, CB = std::move(Callback)](Node Response, Handler::EndCallback End)
                            {
                                if(Response.Id == Identity.Id)
                                {
                                    // All nodes resode before my key

                                    End();
                                    return;
                                }
                                
                                Add(Response);

                                // @todo also not all nodes exist and need to be routed

                                bool Found = false;
                                size_t i;

                                for (i = NthNeighbor - 1; i > 0; i--)
                                {
                                    if(Identity.Id.Neighbor(i) < Response.Id)
                                    {
                                        Found = true;
                                        break;
                                    }
                                }

                                if(!Found)
                                {
                                    i = Identity.Id.Critical();
                                }

                                if(i > NthNeighbor)
                                {
                                    // Wrapped around and bootstrap is done

                                    End();
                                    return;
                                }                                

                                Bootstrap(Closest(Identity.Id.Neighbor(i)).EndPoint, i, std::move(CB), std::move(End));
                            },
                            std::move(End));
                    }

                    void Bootstrap(const Network::EndPoint& Peer, BootstrapCallback Callback, Handler::EndCallback End)
                    {
                        Bootstrap(
                            Peer,
                            Identity.Id.Size,
                            std::move(Callback),
                            std::move(End));
                    }

                    typedef std::function<void(Iterable::Span<char> &Data, Handler::EndCallback)> GetCallback;

                    void GetFrom(Network::EndPoint Peer, const Key& key, GetCallback Callback, Handler::EndCallback End)
                    {
                        Build(
                            Peer,

                            // Build buffer
                            // Get - Key

                            [&key](Format::Serializer &Serializer)
                            {
                                Serializer << (char)Operations::Get << key;
                            },

                            // Process response
                            // Response - Data

                            [this, CB = std::move(Callback)](Network::DHT::Request &request, Handler::EndCallback End)
                            {
                                Format::Serializer Serializer(request.Buffer);

                                char Header;

                                Serializer >> Header;

                                Iterable::Span<char> Data(Serializer.Length());

                                Serializer >> Data;

                                CB(Data, End);
                            },
                            std::move(End));
                    }

                    void Get(Key key, GetCallback Callback, Handler::EndCallback End)
                    {
                        Route(
                            key,
                            [this, key, CB = std::move(Callback)](Node Target, Handler::EndCallback End)
                            {
                                GetFrom(Target.EndPoint, key, CB, std::move(End));
                            },
                            std::move(End));
                    }

                    typedef std::function<void(Handler::EndCallback)> SetCallback;

                    void SetTo(Network::EndPoint Peer, Key key, const Iterable::Span<char> &Data)
                    {
                        Fire(
                            Peer,

                            // Build buffer
                            // Set - Key - Data

                            // No response (yet)

                            [&key, &Data](Format::Serializer &Serializer)
                            {
                                Serializer << (char)Operations::Set << key << Data;
                            });
                    }

                    void Set(const Key& key, const Iterable::Span<char> &Data, Handler::EndCallback End)
                    {
                        Route(
                            key,
                            [this, key, Data](Node Target, Handler::EndCallback End)
                            {
                                SetTo(Target.EndPoint, key, Data);
                                
                                // fire and forget
                                
                                End();
                            },
                            std::move(End));
                    }
                };
            }
        }
    }
}