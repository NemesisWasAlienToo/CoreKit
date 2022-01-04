#pragma once

#include <iostream>
#include <string>
#include <thread>

#include "Network/EndPoint.cpp"
#include "Network/DHT/Key.cpp"

using namespace Core;

namespace Core
{
    namespace Network
    {
        namespace DHT
        {
            struct Node
            {
                // ### Types

                enum class States : char
                {
                    Present = 0,
                    Absent,
                };

                // ### Variables

                Key Id;
                Network::EndPoint EndPoint;
                States State;

                // ### Constructors

                Node() = default;

                Node(size_t KeySize) : Id(KeySize) {}

                Node(Key id, Network::EndPoint endPoint, States state = States::Present) : Id(id), EndPoint(endPoint), State(state) {}

                Node(Node &&Other) : Id(std::move(Other.Id)), EndPoint(Other.EndPoint), State(Other.State) {}

                Node(const Node &Other) : Id(Other.Id), EndPoint(Other.EndPoint), State(Other.State) {}

                // Operators

                Node &operator=(Node &&Other)
                {
                    Id = std::move(Other.Id);
                    EndPoint = Other.EndPoint;
                    State = Other.State;

                    return *this;
                }

                Node &operator=(const Node &Other)
                {
                    Id = Other.Id;
                    EndPoint = Other.EndPoint;
                    State = Other.State;

                    return *this;
                }

                bool operator==(const Node &Other) const
                {
                    return Id == Other.Id &&
                           EndPoint == Other.EndPoint &&
                           State == Other.State;
                }

                bool operator!=(const Node &Other) const
                {
                    return Id != Other.Id ||
                           EndPoint != Other.EndPoint ||
                           State != Other.State;
                }
            };
        }
    }
}