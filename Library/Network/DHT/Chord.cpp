#pragma once

#include <iostream>
#include <string>
#include <type_traits>
#include <time.h>

#include "Network/EndPoint.cpp"
#include "Network/Socket.cpp"
#include "Iterable/List.cpp"
#include "Iterable/Span.cpp"

using namespace Core;

namespace Core
{
    namespace Network
    {
        namespace DHT
        {
            namespace Chord
            {
                template <uint16_t Length>
                struct Key
                {
                    // ### Guards

                    static_assert(Length % 8 == 0, "Length must be a multiple of byte");

                    // ### Constants

                    constexpr static uint16_t Size = Length;

                    // ### Variables

                    char Data[Length / 8];

                    // ### Functions

                    void CopyFrom(char *Source)
                    {
                        for (size_t i = 0; i < Size; i++)
                        {
                            Data[i] = Source[i];
                        }
                    }

                    void CopyTo(char *Destination)
                    {
                        for (size_t i = 0; i < Size; i++)
                        {
                            Destination[i] = Data[i];
                        }
                    }
                };

                template <typename TKey>
                struct Request
                {
                    enum RequestType
                    {
                        Ping = 0,
                        Survey,
                        Get,
                        Set,
                    };

                    uint8_t Type;
                    TKey Key;
                    void *Data;

                    Request() = default;
                    Request(uint8_t type, TKey key, void *data = NULL) : Type(type), Key(key), Data(data) {}

                    Iterable::Span<char> Serialize()
                    {
                        //
                    }
                };

                template <typename TKey>
                struct Node
                {
                    // ### Types

                    typedef uint32_t NodeStatus;

                    enum StatusTypes
                    {
                        Avalaivle = 0,
                    };

                    // ### Variables

                    TKey Id;
                    Network::EndPoint Peer;
                    NodeStatus Status;
                };

                template <typename TKey>
                class Runner
                {
                private:
                    // ### Types

                    // ### Variables

                public:
                    // ### Constructors

                    Runner() {}

                    Network::EndPoint Route(TKey Key)
                    {
                        //
                    }

                    Network::EndPoint Flood(TKey Key)
                    {
                        //
                    }

                    // ### Static functions

                    static int Ping(Network::EndPoint Target, int TimeOut)
                    {
                        char Buffer = 0;

                        Network::Socket socket(Network::Socket::IPv4, Network::Socket::TCP | Network::Socket::NonBlocking);

                        try
                        {
                            socket.Connect(Target);
                        }
                        catch(const std::exception& e)
                        {
                            std::cerr << e.what() << '\n';
                            return -1;
                        }

                        int Result = socket.Await(Network::Socket::Out, TimeOut);

                        if (Result = 0)
                        {
                            return -1;
                        }

                        socket.Send(&Buffer, 1);

                        // Save time

                        clock_t Time = clock();

                        Result = socket.Await(Network::Socket::In, TimeOut);

                        if (Result = 0)
                        {
                            return -1;
                        }

                        // Measure time

                        Time = clock() - Time;

                        return Time / (CLOCKS_PER_SEC / 1000);
                    }

                    static Network::EndPoint Survey(const Network::EndPoint &Target, const TKey &Id)
                    {
                        // Form request

                        // std::tuple

                        Network::Socket socket(Network::Socket::IPv4, Network::Socket::TCP | Network::Socket::NonBlocking);

                        socket.Connect(Target);

                        size_t Size = sizeof(size_t) + sizeof(TKey) + 1;

                        uint8_t Packet[Size];

                        socket.Await(Network::Socket::Out);

                        // Send request

                        // Read response

                        // Format response

                        socket.Close();

                        // return response;
                    }

                    static Network::EndPoint Get(Network::EndPoint Target)
                    {
                        //
                    }

                    static Network::EndPoint Set(Network::EndPoint Target)
                    {
                        //
                    }
                };
            }
        }
    }
}