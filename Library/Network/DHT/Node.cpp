#pragma once

#include <iostream>
#include <string>
#include <thread>

#include "Network/EndPoint.cpp"
#include "Network/DHT/Key.cpp"
#include "Network/DHT/Server.cpp"
#include "Network/DHT/Handler.cpp"
#include "Iterable/List.cpp"
#include "Test.cpp"

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

                enum class Status
                {
                    Available = 0,
                };

                // ### Variables

                Key Id;
                Network::EndPoint EndPoint;
                Status Status;

                // ### Constructors

                Node() = default;

                Node(Key id, Network::EndPoint endPoint) : Id(id), EndPoint(endPoint), Status(Status::Available) {}

                Node(Node&& Other) : Id(std::move(Other.Id)), EndPoint(Other.EndPoint), Status(Other.Status) {}
                
                Node(const Node& Other) : Id(Other.Id), EndPoint(Other.EndPoint), Status(Other.Status) {}

                // Operators

                Node& operator=(Node&& Other)
                {
                    Id = std::move(Other.Id);
                    EndPoint = Other.EndPoint;
                    Status = Other.Status;

                    return *this;
                }

                Node& operator=(const Node& Other)
                {
                    Id = Other.Id;
                    EndPoint = Other.EndPoint;
                    Status = Other.Status;

                    return *this;
                }
            };
        }
    }
}