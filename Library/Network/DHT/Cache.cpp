/**
 * @file Cache.cpp
 * @author Nemesis (github.com/NemesisWasAlienToo)
 * @brief
 * @todo Add multiple layer buffer
 * @todo Resolve must return a list of resolved values
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

#include <Network/EndPoint.cpp>
#include <Network/DHT/Node.cpp>
#include <Iterable/Span.cpp>
#include <Iterable/List.cpp>

namespace Core
{
    namespace Network
    {
        namespace DHT
        {
            class Cache
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
                Iterable::Span<Iterable::List<Node>> Entries;

                Cache() = default;

                Cache(const Key &Identity) : Entries((Identity.Size * 8) + 1)
                {
                    BreakPoint = Identity.Critical();

                    Entries.ForEach(
                        [](Iterable::List<Node> &ent)
                        {
                            ent = Iterable::List<Node>(1);
                        });

                    Entries[0].Add({Identity, {"0.0.0.0:0"}});
                }

                ~Cache() = default;

                // Funtionalities

                const Node& Identity()
                {
                    return Entries[0][0];
                }

                const Iterable::List<Node>& Terminate()
                {
                    return Entries[0];
                }
                
                size_t NeighborHood(const Key& key)
                {
                    return (key - Identity().Id).MSNB();
                }

                const Iterable::List<Node>& Resolve(const Key &key)
                {
                    size_t Index;
                    bool Found = false;

                    size_t Biggest;
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

                bool Add(const Node &node)
                {
                    auto NeighborHood = (node.Id - Identity().Id).MSNB();

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
                }

                void ForEach(const std::function<void(const Node&)>& Callback)
                {
                    Entries.ForEach(
                        [this, &Callback](const Iterable::List<Node>& entry)
                        {
                            if(entry.Length() > 0)
                            {
                                Callback(entry[0]); // @todo Fix this
                            }
                        }
                    );
                }
            };

        }
    }
}