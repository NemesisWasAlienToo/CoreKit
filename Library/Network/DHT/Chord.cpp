/**
 * @file Chord.cpp
 * @author Nemesis (github.com/NemesisWasAlienToo)
 * @brief Chord node runner
 *
 *      Supported operations: [ Response, Invalid, Ping, Query, Set, Get, Data ]
 *
 *      The packet format for each operation is as follows :
 *
 *      Response   : [ Header - Size ] - Response Specifier - Data
 *      Invalid    : [ Header - Size ] - Invalid Specifier - Cause of failure
 *      Ping       : [ Header - Size ] - Ping Specifier - My ID
 *      Query      : [ Header - Size ] - Query Specifier - Target ID -> [ Header - Size ] - Response Specifier - Node
 *      Set        : [ Header - Size ] - Set Specifier - Key - Data
 *      Get        : [ Header - Size ] - Get Specifier - Key
 *      Data       : [ Header - Size ] - Data Specifier - Data
 *
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
#include "Network/DHT/Key.cpp"
#include "Network/DHT/Server.cpp"
#include "Network/DHT/Handler.cpp"
#include "Network/DHT/Node.cpp"
#include "Iterable/List.cpp"
#include "Iterable/Span.cpp"
#include "Iterable/Poll.cpp"
#include "Format/Serializer.cpp"
#include "Duration.cpp"
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
                public:
                    enum class Operations : char
                    {
                        Response = 0,
                        Invalid,
                        Notify,
                        Ping,
                        Query,
                        Set,
                        Get,
                        Data,
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

                public:
                    Iterable::List<Node> Nodes;

                private:
                    std::function<void()> _PoolTask =
                        [this]()
                    {
                        // @todo Clear running event

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
                                {
                                    // Extract peer key and add it

                                    Format::Serializer Ser(Request.Buffer);
                                    Node node(Identity.Id.Size);

                                    node.EndPoint = Request.Peer;
                                    Ser >> node.Id;

                                    Add(node);

                                    Task(node, Ser, End);
                                }
                                else
                                {
                                    Respond(Request);
                                }
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
                        Node node(Identity.Id.Size);
                        char Type;

                        node.EndPoint = Request.Peer;
                        ReqSerializer >> node.Id;

                        // Handle this new node

                        Add(node);

                        // Get request type

                        ReqSerializer >> Type;

                        switch (static_cast<Operations>(Type))
                        {
                        case Operations::Ping:
                        {
                            Test::Log("Ping") << Request.Peer << std::endl;

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

                                    Response << Resolve(key);
                                });

                            Test::Log("Query") << Request.Peer << std::endl;

                            break;
                        }
                        case Operations::Set:
                        {
                            // Set - Key - Data

                            Key key(Identity.Id.Size);
                            ReqSerializer >> key;

                            Iterable::Span<char> Data = ReqSerializer.Blob();

                            OnSet(key, Data);

                            Test::Log("Set") << Request.Peer << std::endl;
                            break;
                        }
                        case Operations::Get:
                        {
                            // Get - Key

                            Key key(Identity.Id.Size);
                            ReqSerializer >> key;

                            OnGet(key);

                            Test::Log("Get") << Request.Peer << std::endl;
                            break;
                        }
                        case Operations::Data:
                        {
                            Iterable::Span<char> Data = ReqSerializer.Blob();

                            OnData(Data);

                            Test::Log("Data") << Request.Peer << std::endl;
                            break;
                        }
                        case Operations::Notify:
                        {
                            //

                            Test::Log("Notify") << Request.Peer << std::endl;
                            break;
                        }
                        case Operations::Invalid:
                        {
                            std::string Cause;

                            // OnFail();

                            ReqSerializer >> Cause;

                            Test::Log("Invalid") << Request.Peer << " : " << Cause << std::endl;
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

                            Test::Log("Unknown") << Request.Peer << " : " << Request.Buffer << std::endl;
                            break;
                        }
                        }
                    }

                public:
                    // ### Public Variables

                    Duration TimeOut;

                    // Request Handlers

                    std::function<void(const Key &)> OnGet;
                    std::function<void(const Key &, const Iterable::Span<char> &)> OnSet;
                    std::function<void(Iterable::Span<char>)> OnData;
                    std::function<void(Iterable::Span<char>)> OnFlood;

                    // ### Constructors

                    Runner() = default;

                    Runner(const Node &identity, Duration Timeout, size_t ThreadCount = 1) : Identity(identity), Server(identity.EndPoint), Handler(), Pool(ThreadCount, false), Nodes(Identity.Id.Size + 1), TimeOut(Timeout)
                    {
                        Nodes.Add({Identity.Id, {"0.0.0.0:0"}});
                    }

                    // ### Static functions

                    inline static Network::EndPoint DefaultEndPoint()
                    {
                        return {"0.0.0.0:8888"};
                    }

                    // ### Functionalities

                    Node &Resolve(const Key &key)
                    {
                        size_t Index;

                        if (Nodes.ContainsWhere(
                                Index,
                                [this, &key](Node &Item) -> bool
                                {
                                    return Item.Id < key;
                                }))
                        {
                            return Nodes[Index];
                        }
                        else
                        {
                            return Nodes.First();
                        }
                    }

                    size_t ResolveAt(const Key &key)
                    {
                        size_t Index;

                        if (!Nodes.ContainsWhere(
                                Index,
                                [this, &key](Node &Item) -> bool
                                {
                                    return Item.Id < key;
                                }))
                        {
                            Index = 0;
                        }

                        return Index;
                    }

                    void Add(Node &&node)
                    {
                        size_t Index;

                        if (Nodes.Contains(node))
                            return;

                        if (Nodes.ContainsWhere(
                                Index,
                                [this, &node](Node &Item) -> bool
                                {
                                    return Item.Id <= node.Id;
                                }))
                        {
                            if (Nodes.IsFull())
                                Nodes[Index] = std::move(node);
                            else
                                Nodes.Squeeze(std::move(node), Index);
                        }
                        else
                        {
                            if (Nodes.IsFull())
                                Nodes.Last() = std::move(node);
                            else
                                Nodes.Add(std::move(node));
                        }
                    }

                    void Add(const Node &node) // @todo Improve this
                    {
                        size_t Index;

                        if (Nodes.Contains(node))
                            return;

                        if (Nodes.ContainsWhere(
                                Index,
                                [this, &node](Node &Item) -> bool
                                {
                                    return Item.Id <= node.Id;
                                }))
                        {
                            if (Nodes.IsFull())
                                Nodes[Index] = node;
                            else
                                Nodes.Squeeze(node, Index);
                        }
                        else
                        {
                            if (Nodes.IsFull())
                                Nodes.Last() = node;
                            else
                                Nodes.Add(node);
                        }
                    }

                    void Await()
                    {
                        //
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

                        for (Index = 0; Index < Nodes.Length() && Nodes[Index].Id > key; Index++)
                        {
                        }

                        return Nodes[Index];
                    }

                    // const Iterable::List<Node> &Nodes() { return Nodes; }

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

                            [this, SendTime = DateTime::Now(), CB = std::move(Callback)](Node& key, Format::Serializer &request, Handler::EndCallback End)
                            {
                                // @todo check retuened id as well

                                CB(DateTime::Now() - SendTime, End);
                                End();
                            },
                            End);
                    }

                    typedef std::function<void(Node, Handler::EndCallback)> QueryCallback;

                    void Query(const Network::EndPoint &Peer, const Key &Id, QueryCallback Callback, Handler::EndCallback End)
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

                            [this, CB = std::move(Callback)](Node& Requester, Format::Serializer &Serializer, Handler::EndCallback End)
                            {
                                if(Requester.Id == Identity.Id)
                                {
                                    End();
                                    return;
                                }

                                char Header;
                                Node node(Identity.Id.Size); // <-- @todo clarify this

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

                    void Route(const Network::EndPoint &Peer, const Key &Id, RouteCallback Callback, Handler::EndCallback End)
                    {
                        Query( // @todo optimize functions in the argument
                            Peer,
                            Id,
                            [this, Peer, Id, CB = std::move(Callback)](Node Response, const Handler::EndCallback &End)
                            {
                                if (Response.EndPoint.address() == Network::Address("0.0.0.0")) // <-- @todo Optimize this
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
                        if (Nodes.Length() <= 1)
                            throw std::underflow_error("No other node is known");

                        // const auto &Peer = Closest(Id).EndPoint;
                        const auto &Peer = Resolve(Id).EndPoint;

                        Route(Peer, Id, std::move(Callback), std::move(End));
                    }

                    typedef std::function<void(Handler::EndCallback)> BootstrapCallback;

                    void Bootstrap(const Network::EndPoint &Peer, size_t NthNeighbor, BootstrapCallback Callback, Handler::EndCallback End)
                    {
                        // @todo Add the bootstrap node after asked for its id

                        auto Neighbor = Identity.Id.Neighbor(NthNeighbor);

                        Route(
                            Peer,
                            Neighbor,
                            [this, NthNeighbor, CB = std::move(Callback)](Node Response, Handler::EndCallback End)
                            {
                                if (Response.Id == Identity.Id)
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
                                    if (Identity.Id.Neighbor(i) < Response.Id)
                                    {
                                        Found = true;
                                        break;
                                    }
                                }

                                if (!Found)
                                {
                                    i = Identity.Id.Critical();
                                }

                                if (i > NthNeighbor)
                                {
                                    // Wrapped around and bootstrap is done

                                    End();
                                    return;
                                }

                                Network::EndPoint Next;

                                while ((Next = Resolve(Identity.Id.Neighbor(i--)).EndPoint).address() == Network::Address("0.0.0.0") && i > 0)
                                {
                                    //
                                }

                                if (i > 0)
                                {
                                    Bootstrap(Next, i, std::move(CB), std::move(End));
                                }
                                else
                                {
                                    End();
                                }
                            },
                            std::move(End));
                    }

                    inline void Bootstrap(const Network::EndPoint &Peer, BootstrapCallback Callback, Handler::EndCallback End)
                    {
                        Bootstrap(
                            Peer,
                            Identity.Id.Size,
                            std::move(Callback),
                            std::move(End));
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

                            // Process response
                            // Response - Data

                            [this, CB = std::move(Callback)](Node& Requester, Format::Serializer &Serializer, Handler::EndCallback End)
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
                            [this, key, CB = std::move(Callback)](Node Target, Handler::EndCallback End)
                            {
                                GetFrom(Target.EndPoint, key, CB, std::move(End));
                            },
                            std::move(End));
                    }

                    void SetTo(const Network::EndPoint &Peer, const Key &key, const Iterable::Span<char> &Data)
                    {
                        Fire(
                            Peer,

                            // Build buffer
                            // Set - Key - Data

                            [&key, &Data](Format::Serializer &Serializer)
                            {
                                Serializer << (char)Operations::Set << key << Data;
                            });
                    }

                    void Set(const Key &key, const Iterable::Span<char> &Data, Handler::EndCallback End)
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

                    // Send data

                    void SendTo(const Network::EndPoint &Peer, const Iterable::Span<char> &Data)
                    {
                        Fire(
                            Peer,

                            // Build buffer
                            // Data - Data

                            [&Data](Format::Serializer &Serializer)
                            {
                                Serializer << (char)Operations::Data << Data;
                            });
                    }

                    // Flood

                    void Flood(const Network::EndPoint &Peer,const Key& key, const Iterable::Span<char> &Data)
                    {
                        Nodes.ForEach(
                            [this, &Data](Node &Node)
                            {
                                Fire(
                                    Node.EndPoint,

                                    // Build buffer
                                    // Data - Data

                                    [this, &Data](Format::Serializer &Serializer)
                                    {
                                        Serializer << (char)Operations::Data << Data;
                                    });
                            });
                    }

                    void FloodWhere(const Network::EndPoint &Peer, const Key& key, const Iterable::Span<char> &Data,
                                    const std::function<bool(const Node &)> &Condition)
                    {
                        Nodes.ForEach(
                            [this, &Data, &Condition](Node &node)
                            {
                                if (Condition(node))
                                    Fire(
                                        node.EndPoint,

                                        // Build buffer
                                        // Data - Data

                                        [this, &Data](Format::Serializer &Serializer)
                                        {
                                            Serializer << (char)Operations::Data << Data;
                                        });
                            });
                    }
                };
            }
        }
    }
}