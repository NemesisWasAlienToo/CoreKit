/**
 * @file Chord.cpp
 * @author Nemesis (github.com/NemesisWasAlienToo)
 * @brief
 * @todo Add multiple layer buffer
 * @todo Resolve must return a list of resolved values
 * @todo Prevent an endpoint from having multiple identities
 * @version 0.1
 * @date 2022-01-11
 *
 * @copyright Copyright (c) 2022
 *
 */

#pragma once

#include <iostream>
#include <string>
#include <mutex>

#include <Network/EndPoint.hpp>
#include <Network/DHT/DHT.hpp>
#include <Network/DHT/Node.hpp>
#include <Iterable/Span.hpp>
#include <Iterable/List.hpp>

namespace Core
{
    namespace Network
    {
        namespace DHT
        {
            class Chord
            {
            private:
                size_t BreakPoint;

                inline Iterable::List<Node> &Ordered(size_t Index)
                {
                    return Entries[(Index + BreakPoint + 1) % Entries.Length()];
                }

                inline size_t UnOrdered(size_t Index)
                {
                    return (Index + (Entries.Length() - (BreakPoint + 1))) % Entries.Length();
                }

            public:
                std::function<void(Node, std::function<void()>)> OnTest;
                Iterable::Span<Iterable::List<Node>> Entries;

                Chord() = default;

                Chord(const Cryptography::Key &Identity) : Entries((Identity.Size * 8) + 1)
                {
                    BreakPoint = Identity.Critical();

                    Entries.ForEach(
                        [](Iterable::List<Node> &ent)
                        {
                            ent = Iterable::List<Node>(1);
                        });

                    Entries[0].Add(DHT::Node{Identity, {"0.0.0.0:0"}});
                }

                ~Chord() = default;

                // Funtionalities

                const Node &Identity()
                {
                    return Entries[0][0];
                }

                const Iterable::List<Node> &Terminate()
                {
                    return Entries[0];
                }

                size_t NeighborHood(const Cryptography::Key &key)
                {
                    return (key - Identity().Id).MSNB();
                }

                const Iterable::List<Node> &Resolve(const Cryptography::Key &key)
                {
                    size_t Index;
                    bool Found = false;

                    size_t Biggest = 0;
                    bool BFournd = false;

                    Iterable::List<Node> Res;

                    // Find the first smaller key

                    for (long i = Entries.Length() - 1; i >= 0; i--)
                    {
                        auto &entry = Ordered(i);

                        if (!entry.Length())
                        {
                            continue;
                        }

                        if (!BFournd)
                        {
                            Biggest = i;
                            BFournd = true;
                        }

                        if (entry[0].Id < key) // <-- @todo Fix misbehave here
                        {
                            Found = true;
                            Index = i;
                            break;
                        }
                    }

                    if (!Found)
                    {
                        Index = Biggest;
                    }

                    return Ordered(Index);
                }

                bool Remove(const Node &node)
                {
                    auto NeighborHood = (node.Id - Identity().Id).MSNB();

                    auto &Neighbor = Entries[NeighborHood];

                    if (auto Index = Neighbor.Contains(
                            [&node](const Node &Item)
                            {
                                return Item.EndPoint == node.EndPoint;
                            }))
                    {
                        // For now just swap it with the last element

                        Neighbor[Index.value()] = Neighbor.Take();

                        return true;
                    }

                    return false;
                }

                /*bool Remove(const EndPoint &endPoint, size_t NeighborHood)
                {
                    auto &Neighbor = Entries[NeighborHood];

                    size_t Index;

                    if (Neighbor.ContainsWhere(
                            Index,
                            [&endPoint](Node &Item)
                            {
                                return Item.EndPoint == endPoint;
                            }))
                    {
                        Neighbor.Remove(Index);
                        return true;
                    }

                    return false;
                }*/

                bool Remove(const EndPoint &endPoint)
                {
                    for (size_t i = 0; i < Entries.Length(); i++)
                    {
                        auto &Neighbor = Entries[i];

                        if (auto Index = Neighbor.Contains(
                                [&endPoint](Node const &Item)
                                {
                                    return Item.EndPoint == endPoint;
                                }))
                        {
                            // For now just swap it with the last element

                            Neighbor[Index.value()] = Neighbor.Take();

                            return true;
                        }
                    }

                    return false;
                }

                bool Add(const Node &node)
                {
                    auto NeighborHood = (node.Id - Identity().Id).MSNB();

                    auto &Neighbor = Entries[NeighborHood];

                    if (Neighbor.IsFull())
                    {
                        return true;
                    }

                    Neighbor.Add(node);

                    return false;
                }

                /*bool Add(const Node &node, size_t NeighborHood)
                {
                    auto &Neighbor = Entries[NeighborHood];

                    if (Neighbor.IsFull()) // @todo Fix this
                    {
                        Neighbor.Last() = node;

                        return true;
                    }
                    else
                    {
                        Neighbor.Add(node);

                        return false;
                    }
                }*/

                template <typename TCallback>
                void ForEach(TCallback Callback)
                {
                    // Lock here

                    for (size_t i = 1; i < Entries.Length(); i++)
                    {
                        Callback(Entries[i]);
                    }
                }
            };
        }
    }
}