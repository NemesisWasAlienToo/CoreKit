#pragma once

#include <iostream>
#include <string>
#include <thread>
#include <tuple>

#include "Network/EndPoint.hpp"
#include "Network/UDPServer.hpp"
#include "Network/DHT/DHT.hpp"
#include "Iterable/List.hpp"
#include "Iterable/Span.hpp"
#include "Iterable/Poll.hpp"

namespace Core
{
    namespace Network
    {
        namespace DHT
        {
            template <class TCache>
            class LightRunner
            {
            public:
                using EndCallback = UDPServer::EndCallback;

            private:
                enum class States : char
                {
                    Stopped = 0,
                    Running,
                };

                Network::DHT::Node Identity;
                Iterable::Span<std::thread> Pool;
                Duration TimeOut;
                Network::UDPServer Server;
                volatile States State;

                TCache Cache;

            public:
                // Variables

                std::function<void(const Node &, Format::Serializer &)> OnData = {};

                // Constructors

                LightRunner() = default;

                LightRunner(const Network::DHT::Node identity, Duration Timeout) : Identity(identity), TimeOut(Timeout), Server(identity.EndPoint), State(States::Stopped), Cache(Identity.Id)
                {
                    Server.Builder = [this](const EndPoint &Peer)
                    {
                        return BuildIEntry(
                            Peer,
                            nullptr,
                            [this](const Network::DHT::Node &Node, Format::Serializer &Serializer, UDPServer::EndCallback &End)
                            {
                                Respond(Node, Serializer);
                            });
                    };

                    Server.OnClean = [this](const Network::EndPoint &EP)
                    {
                        Cache.Remove(EP);
                    };
                }

                // Private functionalities

            private:
                void Respond(const Network::DHT::Node &Node, Format::Serializer &Serializer)
                {
                    Cache.Add(Node);

                    Network::DHT::Operations Type = static_cast<Network::DHT::Operations>(Serializer.Take<char>());

                    switch (Type)
                    {
                    case Network::DHT::Operations::Ping:
                    {
                        SendTo(
                            Node.EndPoint,
                            [](Format::Serializer &Ser) {},
                            {});

                        break;
                    }
                    case Operations::Query:
                    {
                        Cryptography::Key key;

                        Serializer >> key;

                        SendTo(
                            Node.EndPoint,
                            [this, &key](Format::Serializer &Serializer)
                            {
                                Serializer << (char)Operations::Response << Cache.Resolve(key);
                            },
                            {});

                        break;
                    }
                    case Operations::Data:
                    {
                        OnData(Node, Serializer);

                        break;
                    }
                    default:
                    {
                        Test::Warn("Unknown command") << Node.Id << "@" << Node.EndPoint << std::endl;

                        SendTo(
                            Node.EndPoint,
                            [](Format::Serializer &Response)
                            {
                                Response << (char)Operations::Invalid << "Invalid command";
                            },
                            nullptr);

                        break;
                    }
                    }
                }

                // Public functionalities

            public:
                void GetInPool()
                {
                    while (State == States::Running)
                    {
                        try
                        {
                            Server.Loop();
                        }
                        catch (const std::exception &e)
                        {
                            std::cout << e.what() << '\n';
                            break;
                        }
                    }

                    std::cout << "Thread exited" << std::endl;
                }

                inline void Run(size_t ThreadCount)
                {
                    Pool = Iterable::Span<std::thread>(ThreadCount);

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

                    // Join threads in pool

                    Pool.ForEach(
                        [](std::thread &Thread)
                        {
                            Thread.join();
                        });
                }

                // Fundation

                template <class TBuilder>
                UDPServer::Entry BuildOEntry(
                    const Core::Network::EndPoint &Peer,
                    EndCallback End,
                    TBuilder Builder)
                {
                    Iterable::Queue<char> Buffer;

                    Format::Serializer Serializer(Buffer);

                    Serializer.Add(reinterpret_cast<const char *>("CHRD"), 4) << static_cast<uint32_t>(0u);

                    Builder(Serializer);

                    // Check size for being too big

                    Serializer.Modify<uint32_t>(4) = Format::Serializer::Order(static_cast<uint32_t>(Buffer.Length()));

                    return UDPServer::Entry{
                        DateTime::FromNow(TimeOut),
                        std::move(End),
                        [Peer, QU = std::move(Buffer)](const Network::Socket &Socket, UDPServer::EndCallback &End) mutable -> std::tuple<bool, UDPServer::ProductCallback>
                        {
                            QU.Free(Socket.SendTo(&QU.First(), QU.Length(), Peer));

                            if (QU.IsEmpty())
                            {
                                return std::make_tuple(true, std::move(End));
                            }

                            return std::make_tuple(false, nullptr);
                        }};
                }

                template <class TCallback>
                inline UDPServer::ProductCallback IProduct(
                    const Network::EndPoint &Peer,
                    TCallback Callback,
                    Iterable::Queue<char> &Queue,
                    UDPServer::EndCallback &End)
                {
                    return [CB = std::move(Callback), Peer, Q = std::move(Queue), End = std::move(End)]() mutable
                    {
                        Format::Serializer Ser(Q);
                        Network::DHT::Node Node;
                        Node.EndPoint = Peer;
                        Node.Id = Ser.Take<Cryptography::Key>();
                        CB(Node, Ser, End);
                    };
                }

                template <class TCallback>
                inline auto BuildIEntry(
                    const Core::Network::EndPoint &Peer,
                    EndCallback End,
                    TCallback Callback)
                {
                    // @todo Change ProductCallback to lambda and template

                    return UDPServer::Entry{
                        DateTime::FromNow(TimeOut),
                        std::move(End),
                        [this, Queue = Iterable::Queue<char>(), Peer, _Callback = std::move(Callback)](const Network::Socket &Socket, UDPServer::EndCallback &End) mutable -> std::tuple<bool, UDPServer::ProductCallback>
                        {
                            constexpr size_t Padding = 8;

                            if (Queue.Capacity() == 0)
                            {
                                auto [Data, p] = Socket.ReceiveFrom();

                                if (Data[0] != 'C' || Data[1] != 'H' || Data[2] != 'R' || Data[3] != 'D')
                                {
                                    return std::tuple(true, nullptr); // <- Maybe build error emitter
                                }

                                // Get Size

                                size_t Size = Format::Serializer::Order(*reinterpret_cast<uint32_t *>(&Data[4]));

                                if (Size < Padding)
                                {
                                    return std::tuple(false, nullptr);
                                }

                                // Build Queue

                                Queue = Iterable::Queue<char>(Size - Padding, false);

                                Queue.Add(Data.Content() + Padding, Data.Length() - Padding);

                                if (Queue.IsFull())
                                {
                                    return std::tuple(true, IProduct(Peer, _Callback, Queue, End));
                                }

                                return std::tuple(false, nullptr);
                            }
                            else
                            {
                                auto [Data, p] = Socket.ReceiveFrom();

                                // Fill Queue

                                Queue.Add(Data.Content(), Data.Length());

                                if (Queue.IsFull())
                                {
                                    return std::tuple(true, IProduct(Peer, _Callback, Queue, End));
                                }

                                return std::tuple(false, nullptr);
                            }
                        }};
                }

                template <class TBuilder>
                inline void SendTo(const EndPoint &Peer, TBuilder Builder, UDPServer::EndCallback End)
                {
                    Server.SendTo(
                        BuildOEntry(
                            Peer,
                            std::move(End),
                            [this, &Builder](Format::Serializer &Ser)
                            {
                                Ser << Identity.Id;
                                Builder(Ser);
                            }));
                }

                template <class TCallback>
                inline bool ReceiveFrom(const EndPoint &Peer, TCallback Callback, UDPServer::EndCallback End)
                {
                    return Server.ReceiveFrom(Peer, BuildIEntry(Peer, std::move(End), std::move(Callback)));
                }

                template <class TBuilder, class TCallback>
                inline bool Build(const EndPoint &Peer,
                                  const TBuilder &Builder,
                                  TCallback Callback,
                                  EndCallback End)
                {
                    if (!ReceiveFrom(Peer, std::move(Callback), std::move(End)))
                    {
                        return false;
                    }

                    SendTo(Peer, Builder, nullptr);

                    return true;
                }

                // Actual funcionalities

                // Callback => void(const Duration&);

                template <class TCallback>
                inline void Ping(const EndPoint &Peer, TCallback Callback, EndCallback End)
                {
                    Build(
                        Peer,
                        [](Format::Serializer &Serializer)
                        {
                            Serializer << (char)Network::DHT::Operations::Ping;
                        },
                        [Start = DateTime::Now(), CB = std::move(Callback)](const Network::DHT::Node &Node, Format::Serializer &Serializer, UDPServer::EndCallback &End)
                        {
                            char Header;

                            Serializer >> Header;

                            CB(DateTime::Now() - Start, std::move(End));
                        },
                        std::move(End));
                }

                template <class TCallback>
                void Query(const Network::EndPoint &Peer, const Cryptography::Key &Id, TCallback Callback, EndCallback End)
                {
                    Build(
                        Peer,

                        // Build buffer

                        [&Id](Format::Serializer &Serializer)
                        {
                            Serializer << (char)Operations::Query << Id;
                        },

                        // Process response

                        [CB = std::move(Callback)](const Network::DHT::Node &Node, Format::Serializer &Serializer, EndCallback End)
                        {
                            char Header;

                            Serializer >> Header;

                            try
                            {
                                Iterable::List<DHT::Node> Nodes;
                                Serializer >> Nodes;
                                CB(std::move(Nodes), std::move(End));
                            }
                            catch (const std::exception &e)
                            {
                                Test::Warn(e.what()) << Serializer << std::endl;
                                End();
                            }
                        },
                        std::move(End));
                }

                template <class TCallback>
                void Route(const Network::EndPoint &Peer, const Cryptography::Key &Id, TCallback Callback, EndCallback End)
                {
                    if (Id == Identity.Id)
                    {
                        Callback(Cache.Terminate(), std::move(End));
                        return;
                    }

                    Query( // @todo optimize functions in the argument
                        Peer,
                        Id,
                        [this, Peer, Id, CB = std::move(Callback)](Iterable::List<Node> Response, EndCallback End)
                        {
                            if (Response[0].Id == Identity.Id)
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
                
                template <class TCallback>
                void Route(const Cryptography::Key &Id, TCallback Callback, EndCallback End)
                {
                    const auto &Peer = Cache.Resolve(Id);

                    if (Peer[0].Id == Identity.Id)
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
                                FillCache(Cache.Resolve(i)[0].EndPoint, i, std::move(End));
                            }
                            else
                            {
                                End();
                            }
                        },
                        std::move(End));
                }

                inline void Bootstrap(const Network::EndPoint &Peer, EndCallback End)
                {
                    FillCache(Peer, Identity.Id.Size, std::move(End));
                }

                inline void Data(const EndPoint &Peer, const Iterable::Span<char> &Data)
                {
                    SendTo(
                        Peer,
                        [&Data](Format::Serializer &Serializer)
                        {
                            Serializer << (char)Network::DHT::Operations::Data << Data;
                        },
                        nullptr);
                }

                inline void Data(const EndPoint &Peer, const std::string &Data)
                {
                    SendTo(
                        Peer,
                        [&Data](Format::Serializer &Serializer)
                        {
                            Serializer << (char)Network::DHT::Operations::Data;

                            Serializer.Add(Data.c_str(), Data.length());
                        },
                        nullptr);
                }
            };
        }
    }
}