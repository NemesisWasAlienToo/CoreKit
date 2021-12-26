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
                Iterable::Queue<char> Buffer;
                Network::EndPoint Peer;
            };
        }
    }
}
