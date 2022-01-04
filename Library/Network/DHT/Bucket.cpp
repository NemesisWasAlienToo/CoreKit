#pragma once

#include <iostream>
#include <string>
#include <thread>

#include <Network/EndPoint.cpp>
#include <Network/DHT/Node.cpp>
#include <Iterable/List.cpp>

using namespace Core;

namespace Core
{
    namespace Network
    {
        namespace DHT
        {
            class Bucket
            {
            private:
                Iterable::List<Node> Nodes;
            public:
                Bucket(size_t KeySize);
                ~Bucket();
            };
            
        }
    }
}