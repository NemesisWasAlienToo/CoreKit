#pragma once

#include <iostream>

#include "Network/EndPoint.cpp"
#include "Iterable/Queue.cpp"

using namespace Core;

namespace Core
{
    namespace Network
    {
        namespace DHT
        {
            struct Request
            {
                Network::EndPoint Peer;
                Iterable::Queue<char> Buffer;

                Request() = default;

                Request(const Network::EndPoint& peer, size_t BufferSize) : Peer(peer), Buffer(BufferSize) {}

                Request(Request &&Other) : Peer(Other.Peer), Buffer(std::move(Other.Buffer)) {}

                Request(const Request &Other) : Peer(Other.Peer), Buffer(Other.Buffer) {}

                Request &operator=(Request &&Other)
                {
                    Peer = Other.Peer;
                    Buffer = std::move(Other.Buffer);

                    return *this;
                }

                Request &operator=(const Request &Other)
                {
                    Peer = Other.Peer;
                    Buffer = Other.Buffer;

                    return *this;
                }
            };
        }
    }
}
