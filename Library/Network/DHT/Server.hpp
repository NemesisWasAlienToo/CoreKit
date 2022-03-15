/**
 * @file Server.cpp
 * @author Nemesis (github.com/NemesisWasAlienToo)
 * @brief
 * @todo Add time out for _Incomming and out going requests
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
#include <thread>
#include <functional>
#include <map>
#include <tuple>

#include "Test.hpp"
#include "Event.hpp"

#include "Network/EndPoint.hpp"
#include "Network/Socket.hpp"

#include "Iterable/List.hpp"
#include "Iterable/Poll.hpp"

#include "Format/Serializer.hpp"
#include "Network/DHT/DHT.hpp"

namespace Core
{
    namespace Network
    {
        namespace DHT
        {
            class Server
            {
            private:
                struct OutEntry
                {
                    EndCallback End;
                    Iterable::Queue<char> Buffer;
                };

                struct InEntry
                {
                    DateTime Expire;
                    EndCallback End;
                    Iterable::Queue<char> Buffer;
                };

                using Map = std::map<Network::EndPoint, InEntry>;
                using Queue = Iterable::Queue<OutEntry>;

                Network::Socket _Socket;

                std::mutex _IncommingLock;
                Iterable::Queue<Network::DHT::Request> _Incomming;
                // Map Incomming;

                std::mutex _OutgoingLock;
                Iterable::Queue<Network::DHT::Request> _Outgoing;
                // Queue Outgoing;

                // Map::iterator Tracking;

            public:
                Server() = default;

                Server(const Network::EndPoint &EndPoint) : _Socket(Network::Socket::IPv4, Network::Socket::UDP | Network::Socket::NonBlocking), _Incomming(1), _Outgoing(1)
                {
                    _Socket.Bind(EndPoint);
                }

                inline Descriptor Listener()
                {
                    return _Socket.INode();
                }

                Iterable::Poll::Event Events()
                {
                    return _Outgoing.IsEmpty() ? (Iterable::Poll::In) : (Iterable::Poll::In | Iterable::Poll::Out);
                }

                void Remove(const EndPoint &Peer)
                {
                    std::lock_guard _Lock(_IncommingLock);

                    size_t Index;

                    if (_Incomming.ContainsWhere(
                            Index,
                            [Peer](Request &Item)
                            {
                                return !Item.Buffer.IsFull() && Item.Peer == Peer;
                            }))
                    {
                        _Incomming.Remove(Index);
                    }
                }

                void Send()
                {
                    std::lock_guard _Lock(_OutgoingLock);

                    if (_Outgoing.IsEmpty())
                        return;

                    auto &Last = _Outgoing.Last();

                    auto [Pointer, Size] = Last.Buffer.Chunk();

                    Last.Buffer.Free(_Socket.SendTo(Pointer, Size, Last.Peer));

                    if (Last.Buffer.IsEmpty())
                        _Outgoing.Take();
                }

                bool Put(const Network::EndPoint &Peer, const std::function<void(Format::Serializer &)> &Builder, size_t Size = 1024)
                {
                    // @todo Optimize and generalize this

                    Request request(Peer, Size + 9 /* 9 is default header size */);

                    Format::Serializer Serializer(request.Buffer);

                    Serializer.Add((char *)"CHRD", 4) << (uint32_t)0;

                    Builder(Serializer);

                    Serializer.Modify<uint32_t>(4) = Format::Serializer::Order((uint32_t)request.Buffer.Length());

                    {
                        std::lock_guard<std::mutex> lock(_OutgoingLock);

                        _Outgoing.Add(std::move(request));
                    }

                    return true;
                }

                bool Take(const std::function<void(Request &)> &OnReady)
                {
                    auto len = _Socket.Received();
                    char Data[len];
                    Network::EndPoint Peer;
                    size_t Index;

                    bool Ret = false;
                    Request request;

                    // Read arrived data

                    len = _Socket.ReceiveFrom(Data, len, Peer); // @todo underflow bug here

                    // If its a fragmented packet

                    {
                        std::lock_guard _Lock(_IncommingLock); // @todo Not needed just to be sure for now

                        if (_Incomming.ContainsWhere(
                                Index,
                                [Peer](Request &Item)
                                {
                                    return !Item.Buffer.IsFull() && Item.Peer == Peer;
                                }))
                        {
                            auto &Buffer = _Incomming[Index].Buffer;

                            auto readlen = std::min((Buffer.Capacity() - Buffer.Length()), len);

                            Buffer.Add(Data, readlen);
                        }
                        else
                        {
                            // Validate packet header identifier

                            if (Data[0] != 'C' || Data[1] != 'H' || Data[2] != 'R' || Data[3] != 'D')
                                return false;

                            // Get packet length

                            uint32_t capacity = ntohl(*(uint32_t *)(&Data[4])) - 8;

                            Iterable::Queue<char> Buffer(capacity, false);

                            Buffer.Add(&Data[8], len - 8);

                            Request req;

                            req.Peer = Peer;
                            req.Buffer = std::move(Buffer);

                            _Incomming.Add(std::move(req));

                            Index = _Incomming.Length() - 1;

                            // @todo Fix second request
                        }

                        auto &Current = _Incomming[Index];

                        // If its completed move it to

                        if ((Ret = Current.Buffer.IsFull()))
                        {
                            request = _Incomming.Swap(Index);
                        }
                    }

                    if (Ret)
                    {
                        OnReady(request);
                    }

                    return Ret;
                }
            };
        }
    }
}
