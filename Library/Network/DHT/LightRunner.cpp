#pragma once

#include <iostream>
#include <string>
#include <thread>
#include <tuple>

#include "Network/EndPoint.cpp"
#include "Network/UDPServer.cpp"
#include "Network/DHT/DHT.cpp"
#include "Iterable/List.cpp"
#include "Iterable/Span.cpp"
#include "Iterable/Poll.cpp"

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
                Iterable::List<std::thread> Pool;
                Duration TimeOut;
                Network::UDPServer Server;
                volatile States State;

                TCache Cache;

            public:
                // Variables

                std::function<void(const Node &, Iterable::Span<char> &)> OnData = {};

                // Constructors

                LightRunner() = default;

                LightRunner(const Network::DHT::Node identity, Duration Timeout, size_t ThreadCount) : Identity(identity), Pool(ThreadCount, false), TimeOut(Timeout), Server(identity.EndPoint), State(States::Stopped), Cache(Identity.Id)
                {
                    Server.Builder = [this](UDPServer::Map::iterator &Iterator)
                    {
                        Iterator->second =
                            {
                                DateTime::FromNow(TimeOut),
                                nullptr,
                                BuildIHandler(
                                    Iterator,
                                    [this, Iterator](const Network::DHT::Node &Node, Format::Serializer &Serializer, UDPServer::EndCallback &End)
                                    {
                                        Respond(Node, Serializer);
                                    })};
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

                    char Type;

                    Serializer >> Type;

                    switch (static_cast<Network::DHT::Operations>(Type))
                    {
                    case Network::DHT::Operations::Ping:
                    {
                        SendTo(
                            Node.EndPoint,
                            [](Format::Serializer &Ser)
                            {
                                Ser << '\0';
                            },
                            {});

                        break;
                    }
                    case Operations::Query:
                    {
                        Key key;

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
                        Iterable::Span<char> Data = Serializer.Dump();

                        OnData(Node, Data);

                        break;
                    }
                    default:
                    {
                        Test::Warn("Unknown command") << Node.Id << "@" << Node.EndPoint << std::endl;

                        SendTo(
                            Node.EndPoint,
                            [this](Format::Serializer &Response)
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
                        Server.Loop();

                        //     try
                        //     {
                        //         Server.Loop();
                        //     }
                        //     catch (const std::exception &e)
                        //     {
                        //         std::cout << e.what() << '\n';
                        //         break;
                        //     }
                    }

                    std::cout << "Thread exited" << std::endl;
                }

                inline void Run()
                {
                    State = States::Running;

                    Pool.Reserve(Pool.Capacity());

                    for (size_t i = 0; i < Pool.Capacity(); i++)
                    {
                        Pool[i] = std::thread(
                            [this]
                            {
                                GetInPool();
                            });
                    }
                }

                // template<size_t ThreadCount>
                // inline void Run(size_t ThreadCount)

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
                auto BuildOHandler(
                    const Core::Network::EndPoint &Peer,
                    const TBuilder &Builder)
                {
                    Iterable::Queue<char> Buffer;

                    Format::Serializer Serializer(Buffer);

                    Serializer.Add((char *)"CHRD", 4) << (uint32_t)0;

                    Builder(Serializer);

                    Serializer.Modify<uint32_t>(4) = Format::Serializer::Order((uint32_t)Buffer.Length());

                    return [this, Peer, QU = std::move(Buffer)](const Network::Socket &Socket) mutable -> bool
                    {
                        QU.Free(Socket.SendTo(&QU.First(), QU.Length(), Peer));

                        return QU.IsEmpty();
                    };
                }

                // @todo Maybe add OProduct() too?

                template <class TCallback>
                inline UDPServer::ProductCallback IProduct(
                    Network::EndPoint &Peer,
                    TCallback Callback,
                    Iterable::Queue<char> &Queue,
                    UDPServer::EndCallback &End)
                {
                    return [this, CB = std::move(Callback), Peer, Q = std::move(Queue), End = std::move(End)]() mutable
                    {
                        Format::Serializer Ser(Q);
                        Network::DHT::Node Node;
                        Node.EndPoint = Peer;
                        Ser >> Node.Id;
                        CB(Node, Ser, End);
                    };
                }

                template <class TCallback>
                inline auto BuildIHandler(
                    UDPServer::Map::iterator &Iterator,
                    TCallback Callback)
                {
                    // @todo Change ProductCallback to lambda and template

                    return [this, It = Iterator, _Callback = std::move(Callback)](const Network::Socket &Socket, UDPServer::EndCallback &End) mutable -> std::tuple<bool, UDPServer::ProductCallback>
                    {
                        constexpr size_t Padding = 8;

                        auto [Data, Peer] = Socket.ReceiveFrom();

                        if (Data[0] != 'C' || Data[1] != 'H' || Data[2] != 'R' || Data[3] != 'D')
                        {
                            return std::tuple(true, nullptr); // <- Maybe build error emitter
                        }

                        // Get Size

                        uint32_t Lenght = ntohl(*(uint32_t *)(&Data[4])) - Padding;

                        // Build Queue

                        Iterable::Queue<char> Queue(Lenght, false);

                        // Fill Queue

                        Queue.Add(Data.Content() + Padding, Data.Length() - Padding);

                        if (Queue.IsFull())
                        {
                            return std::tuple(true, IProduct(Peer, _Callback, Queue, End));
                        }

                        // Build handler

                        It->second.Handler =
                            [this, QU = std::move(Queue), _CB = std::move(_Callback)](const Network::Socket &Socket, UDPServer::EndCallback &End) mutable -> std::tuple<bool, UDPServer::ProductCallback>
                        {
                            auto [Data, Peer] = Socket.ReceiveFrom();

                            QU.Add(Data.Content(), Data.Length());

                            if (!QU.IsFull())
                            {
                                return std::tuple(false, nullptr);
                            }

                            return std::tuple(true, IProduct(Peer, _CB, QU, End));
                        };

                        return std::tuple(false, nullptr);
                    };
                }

                template <class TBuilder>
                inline void SendTo(const EndPoint &Peer, const TBuilder &Builder, EndCallback End)
                {
                    Server.SendTo(
                        BuildOHandler(
                            Peer,
                            [this, &Builder](Format::Serializer &Ser)
                            {
                                Ser << Identity.Id;
                                Builder(Ser);
                            }),
                        std::move(End));

                    // @todo Due to reallocation memory location might change

                    // Server.SendTo(
                    //     [this, &End, &Peer, &Builder](UDPServer::OutEntry &OEntry)
                    //     {
                    //         OEntry =
                    //             {
                    //                 std::move(End),
                    //                 BuildOHandler(
                    //                     Peer,
                    //                     [this, &Builder](Format::Serializer &Ser)
                    //                     {
                    //                         Ser << Identity.Id;
                    //                         Builder(Ser);
                    //                     })};
                    //     });
                }

                template <class TCallback>
                inline bool ReceiveFrom(
                    const EndPoint &Peer,
                    TCallback Callback,
                    UDPServer::EndCallback End)
                {
                    return Server.ReceiveFrom(
                        Peer,
                        [this, &Callback, &End](UDPServer::Map::iterator &Iterator, bool Inserted)
                        {
                            // @todo Due to reallocation memory location might change

                            if (!Inserted)
                            {
                                return;
                            }

                            Iterator->second =
                                {
                                    DateTime::FromNow(TimeOut),
                                    std::move(End),
                                    BuildIHandler(Iterator, std::move(Callback))};
                        });
                }

                template <class TBuilder, class TCallback>
                inline bool Build(const EndPoint &Peer,
                                  const TBuilder &Builder,
                                  TCallback Callback,
                                  EndCallback End)
                {
                    if (!ReceiveFrom(Peer, std::move(Callback), std::move(End)))
                    {
                        std::cout << "EndPoint is occupied\n";
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

                            CB(DateTime::Now() - Start, End);
                        },
                        std::move(End));
                }

                template <class TCallback>
                void Query(const Network::EndPoint &Peer, const Key &Id, TCallback Callback, EndCallback End)
                {
                    Build(
                        Peer,

                        // Build buffer

                        [&Id](Format::Serializer &Serializer)
                        {
                            Serializer << (char)Operations::Query << Id;
                        },

                        // Process response

                        [this, CB = std::move(Callback)](const Network::DHT::Node &Node, Format::Serializer &Serializer, EndCallback End)
                        {
                            char Header;

                            Serializer >> Header;

                            try
                            {
                                Iterable::List<DHT::Node> Nodes;
                                Serializer >> Nodes;
                                CB(Nodes, End);
                            }
                            catch (const std::exception &e)
                            {
                                Test::Warn(e.what()) << Serializer << std::endl;
                                // End({Report::Codes::InvalidArgument, e.what()});
                                End();
                            }
                        },
                        std::move(End));
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