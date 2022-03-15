/**
 * @file Runner.cpp
 * @author Nemesis (github.com/NemesisWasAlienToo)m
 * 
 * @brief DHT node runner
 *
 *      Supported operations: [ Response, Invalid, Ping, Query, Keys, Set, Get, Data ]
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
 * @todo Optimize changing std::function to template where possible
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

#include "Network/DHT/Node.hpp"
#include "Network/DHT/Server.hpp"
#include "Network/DHT/Handler.hpp"
#include "Iterable/List.hpp"
#include "Iterable/Span.hpp"
#include "Iterable/Poll.hpp"
#include "Format/Serializer.hpp"
#include "Duration.hpp"
#include "Test.hpp"

namespace Core
{
    namespace Network
    {
        namespace DHT
        {
            template <class CachePolicy>
            class Runner
            {
            private:
                // ### Variables

                enum class States : char
                {
                    Stopped = 0,
                    Running,
                };

                Node Identity;

                Network::DHT::Server Server;

                Network::DHT::Handler Handler;

                Event Interrupt;

                std::mutex Lock;

                Iterable::Poll Poll;

                Iterable::List<std::thread> Pool;

                CachePolicy Cache;

                volatile States State;

                void Await()
                {
                    Poll[0].Mask = Server.Events();

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

                                                        Cache.Test(node);
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
                                    Server.Remove(Target);
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
                                Response << (char)Operations::Response;
                            });

                        break;
                    }
                    case Operations::Query:
                    {
                        Cryptography::Key key(Identity.Id.Size);

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
                    case Operations::Keys:
                    {
                        OnKeys(
                            [this, &node, &ReqSerializer](const Iterable::List<Cryptography::Key> &keys)
                            {
                                Fire(
                                    node.EndPoint,
                                    [&keys](Format::Serializer &Serializer)
                                    {
                                        Serializer << (char)Operations::Response << keys;
                                    });
                            });

                        break;
                    }
                    case Operations::Set:
                    {
                        // Set - Key - Data

                        Cryptography::Key key(Identity.Id.Size);
                        ReqSerializer >> key;

                        Iterable::Span<char> Data = ReqSerializer.Dump();

                        OnSet(key, Data);

                        break;
                    }
                    case Operations::Get:
                    {
                        // Get - Key

                        Cryptography::Key key(Identity.Id.Size);
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
                        Iterable::Span<char> Data = ReqSerializer.Dump();

                        OnData(node, Data);

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

                std::function<void(OnKeysCallback)> OnKeys = {};
                std::function<void(const Cryptography::Key &, OnGetCallback)> OnGet = {};
                std::function<void(const Cryptography::Key &, const Iterable::Span<char> &)> OnSet = {};
                std::function<void(const Node &, Iterable::Span<char> &)> OnData = {};

                // ### Constructors

                Runner() = default;

                Runner(const Node &identity, Duration Timeout, size_t ThreadCount = 1) : Identity(identity), Server(identity.EndPoint), Handler(), Poll(3),
                                                                                         Pool(ThreadCount, false), Cache(Identity.Id), State(States::Stopped), TimeOut(Timeout)
                {
                    Cache.OnTest = [this](Node Peer, auto OnFail)
                    {
                        Ping(
                            Peer.EndPoint,
                            [](Duration duration, EndCallback End)
                            {
                                // if it is successfull dont call end
                            },
                            [CB = std::move(OnFail)](const Network::DHT::Report &Report)
                            {
                                Test::Log("Cache") << "Replaced" << std::endl;
                                CB();
                            });
                    };

                    Poll.Add(Server.Listener(), Server.Events());
                    Poll.Add(Handler.Listener(), Timer::In);
                    Poll.Add(Interrupt.INode(), Event::In);
                }

                // ### Functionalities

                // void Await()
                // {
                //     //
                // }

                std::function<void()> GetInPool = [this]()
                {
                    try
                    {
                        EventLoop();
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << e.what() << '\n';
                    }

                    Test::Log("Pool") << "Exited" << std::endl;
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

                inline void Fire(
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

                template<class TBuilder>
                bool Build(
                    const Network::EndPoint &Peer,
                    const TBuilder &Builder,
                    Network::DHT::Handler::Callback Callback,
                    Network::DHT::EndCallback End)
                {
                    if (!Handler.Put(Peer, DateTime::FromNow(TimeOut), std::move(Callback), std::move(End)))
                    {
                        End(Report::Codes::Occupied);
                        Test::Error("Handler") << "Handler already exists" << std::endl;
                        return false;
                    }

                    Fire(Peer, Builder);

                    return true;
                }

                void Ping(const Network::EndPoint &Peer, PingCallback Callback, EndCallback End)
                {
                    Build(
                        Peer,

                        // Build buffer

                        [this](Format::Serializer &Serializer)
                        {
                            Serializer << (char)Operations::Ping;
                        },

                        // Process response

                        [this, SendTime = DateTime::Now(), CB = std::move(Callback)](Node &key, Format::Serializer &request, EndCallback End)
                        {
                            // @todo check retuened id as well

                            CB(DateTime::Now() - SendTime, End);
                        },
                        std::move(End));
                }

                void Query(const Network::EndPoint &Peer, const Cryptography::Key &Id, QueryCallback Callback, EndCallback End)
                {
                    Build(
                        Peer,

                        // Build buffer

                        [&Id](Format::Serializer &Serializer)
                        {
                            Serializer << (char)Operations::Query << Id;
                        },

                        // Process response

                        [this, CB = std::move(Callback)](Node &Requester, Format::Serializer &Serializer, EndCallback End)
                        {
                            if (Requester.Id == Identity.Id)
                            {
                                End({Report::Codes::InvalidArgument, "Self refrence"});
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
                                End({Report::Codes::InvalidArgument, "Invalid address"});
                            }
                        },
                        std::move(End));
                }

                void Route(const Network::EndPoint &Peer, const Cryptography::Key &Id, RouteCallback Callback, EndCallback End)
                {
                    if (Id == Identity.Id)
                    {
                        Callback(Cache.Terminate(), std::move(End));
                        return;
                    }

                    Query( // @todo optimize functions in the argument
                        Peer,
                        Id,
                        [this, Peer, Id, CB = std::move(Callback)](Iterable::List<Node> Response, const EndCallback &End)
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

                void Route(const Cryptography::Key &Id, RouteCallback Callback, EndCallback End)
                {
                    const auto &Peer = Cache.Resolve(Id);

                    [[unlikely]] if (Peer[0].Id == Identity.Id)
                    {
                        Callback(Cache.Terminate(), std::move(End));
                        return;
                    }

                    Route(Peer[0].EndPoint, Id, std::move(Callback), std::move(End));
                }

                void FillCache(const Network::EndPoint &Peer, size_t NthNeighbor, EndCallback End)
                {
                    auto Neighbor = Identity.Id.Neighbor(NthNeighbor);

                    Route(
                        Peer,
                        Neighbor,
                        [this, NthNeighbor](Iterable::List<Node> Response, EndCallback End)
                        {
                            if (Response[0].Id == Identity.Id)
                            {
                                // All nodes reside before my key

                                End({Report::Codes::Normal});
                                return;
                            }

                            // @todo also not all nodes exist and need to be routed

                            size_t i = Cache.NeighborHood(Response[0].Id);

                            if (i > NthNeighbor)
                            {
                                // Wrapped around and bootstrap is done

                                End({Report::Codes::Normal});
                                return;
                            }

                            if (i > 0)
                            {
                                FillCache(Cache.Resolve(i)[0].EndPoint, i, std::move(End));
                            }
                            else
                            {
                                End({Report::Codes::InvalidArgument, "Invalid neighbor number"});
                            }
                        },
                        std::move(End));
                }

                void FillData(EndCallback End)
                {
                    Route(
                        Identity.Id - 1,
                        [this](Iterable::List<Node> Response, EndCallback End)
                        {
                            if (Response.IsEmpty())
                            {
                                End({Report::Codes::Normal});
                                return;
                            }

                            auto &Target = Response.First();

                            if (Target.Id == Identity.Id)
                            {
                                End({Report::Codes::Normal});
                                return;
                            }

                            Keys(
                                Target.EndPoint,
                                [this, Peer = Response.First().EndPoint](const Iterable::List<Cryptography::Key> &Keys, EndCallback End)
                                {
                                    Keys.ForEach(
                                        [this, Peer, &End](const Cryptography::Key &key)
                                        {
                                            if (key >= Identity.Id)
                                                Get(
                                                    key,
                                                    [this, &key](Iterable::Span<char> &Data, EndCallback End)
                                                    {
                                                        OnSet(key, Data);
                                                    },
                                                    {});
                                        });

                                    End({Report::Codes::Normal});
                                },
                                std::move(End));
                        },
                        std::move(End));
                }

                inline void Bootstrap(const Network::EndPoint &Peer, EndCallback End)
                {
                    FillCache(
                        Peer,
                        Identity.Id.Size,
                        [this, End = std::move(End)](const Network::DHT::Report &Report)
                        {
                            FillData(End);
                        });
                }

                void Keys(const Network::EndPoint &Peer, KeysCallback Callback, EndCallback End)
                {
                    Build(
                        Peer,
                        [](Format::Serializer &Serializer)
                        {
                            Serializer << (char)Operations::Keys;
                        },
                        [this, CB = std::move(Callback)](Node &Requester, Format::Serializer &Serializer, EndCallback End)
                        {
                            char Header;

                            Serializer >> Header;

                            Iterable::List<Cryptography::Key> Keys;

                            Serializer >> Keys;

                            CB(Keys, End);
                        },
                        std::move(End));
                }

                void GetFrom(const Network::EndPoint &Peer, const Cryptography::Key &key, GetCallback Callback, EndCallback End)
                {
                    Build(
                        Peer,

                        // Build buffer
                        // Get - Key

                        [&key](Format::Serializer &Serializer)
                        {
                            Serializer << (char)Operations::Get << key;
                        },
                        [this, CB = std::move(Callback)](Node &Requester, Format::Serializer &Serializer, EndCallback End)
                        {
                            char Header;

                            Serializer >> Header;

                            Iterable::Span<char> Data(Serializer.Length());

                            Serializer >> Data;

                            CB(Data, End);
                        },
                        std::move(End));
                }

                void Get(const Cryptography::Key &key, GetCallback Callback, EndCallback End)
                {
                    Route(
                        key,
                        [this, key, CB = std::move(Callback)](Iterable::List<Node> Peers, EndCallback End)
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

                void SetTo(const Network::EndPoint &Peer, const Cryptography::Key &key, const Iterable::Span<char> &Data)
                {
                    Fire(
                        Peer,
                        [&key, &Data](Format::Serializer &Serializer)
                        {
                            Serializer << (char)Operations::Set << key << Data;
                        });
                }

                void Set(const Cryptography::Key &key, const Iterable::Span<char> &Data, EndCallback End)
                {
                    Route(
                        key,
                        [this, key, Data](Iterable::List<Node> Peers, EndCallback End)
                        {
                            if (Peers[0].Id == Identity.Id) // @todo Check all nodes in response
                            {
                                OnSet(key, Data);
                                End({Report::Codes::Normal}); // <-- Fix this
                                return;
                            }

                            SetTo(Peers[0].EndPoint, key, Data);

                            End({Report::Codes::Normal});
                        },
                        std::move(End));
                }

                void SendTo(const Network::EndPoint &Peer, const Iterable::Span<char> &Data)
                {
                    Fire(
                        Peer,
                        [&Data](Format::Serializer &Serializer)
                        {
                            Serializer << (char)Operations::Data << Data;
                        });
                }

                void SendTo(const Network::EndPoint &Peer, const Iterable::Span<char> &Data, SendToCallback Callback, EndCallback End)
                {
                    Build(
                        Peer,
                        [&Data](Format::Serializer &Serializer)
                        {
                            Serializer << (char)Operations::Data << Data;
                        },
                        [this, CB = std::move(Callback)](Node &Requester, Format::Serializer &Serializer, EndCallback End)
                        {
                            char Header;

                            Serializer >> Header;

                            Iterable::Span<char> Data = Serializer.Dump();

                            CB(Data, End);
                        },
                        std::move(End));
                }

                template<class TCondition>
                void SendWhere(const Iterable::Span<char> &Data, const TCondition &Condition)
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
            };
        }
    }
}