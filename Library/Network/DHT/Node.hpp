#pragma once

#include <iostream>
#include <string>
#include <thread>

#include <Network/EndPoint.hpp>
#include <Cryptography/Key.hpp>
#include <Format/Serializer.hpp>

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

                // ### Variables

                Cryptography::Key Id;
                Network::EndPoint EndPoint;

                // ### Constructors

                Node() = default;

                Node(size_t KeySize) : Id(KeySize) {}

                Node(Cryptography::Key id, Network::EndPoint endPoint) : Id(id), EndPoint(endPoint) {}

                Node(Node &&Other) : Id(std::move(Other.Id)), EndPoint(Other.EndPoint) {}

                Node(const Node &Other) : Id(Other.Id), EndPoint(Other.EndPoint) {}

                // Operators

                Node &operator=(Node &&Other)
                {
                    Id = std::move(Other.Id);
                    EndPoint = Other.EndPoint;

                    return *this;
                }

                Node &operator=(const Node &Other)
                {
                    Id = Other.Id;
                    EndPoint = Other.EndPoint;

                    return *this;
                }

                bool operator==(const Node &Other) const
                {
                    return Id == Other.Id &&
                           EndPoint == Other.EndPoint;
                }

                bool operator!=(const Node &Other) const
                {
                    return Id != Other.Id ||
                           EndPoint != Other.EndPoint;
                }

                friend Format::Serializer &operator<<(Format::Serializer &Ser, const Network::DHT::Node &Value)
                {
                    return Ser << Value.Id << Value.EndPoint;
                }

                friend Format::Serializer &operator>>(Format::Serializer &Ser, Network::DHT::Node &Value)
                {
                    return Ser >> Value.Id >> Value.EndPoint;
                }
            };
        }
    }
}