#pragma once

#include <iostream>
#include <string>
#include <mutex>
#include <thread>
#include <functional>

#include "Event.cpp"

#include "Network/EndPoint.cpp"
#include "Network/Socket.cpp"

#include "Iterable/List.cpp"
#include "Iterable/Poll.cpp"

#include <Format/Serializer.cpp>
#include "Network/DHT/Request.cpp"

using namespace Core;

namespace Core
{
    namespace Network
    {
        namespace DHT
        {
            class Server
            {
            private:
                Network::Socket _Socket;
                Event _Interrupt;

                std::thread Runner;

                // For processing requests

                Iterable::Queue<Request> _Sends;
                Iterable::Queue<Request> _Receives;

                // For new requests

                std::mutex IncommingLock;
                Iterable::Queue<Network::DHT::Request> Incomming;

                std::mutex OutgoingLock;
                Iterable::Queue<Network::DHT::Request> Outgoing;

            public:
                enum class States : uint8_t
                {
                    Running,
                    Stopped,
                };

                volatile States State;

                Event Ready;

                Server() = default;

                Server(const Network::EndPoint& EndPoint) : _Socket(Network::Socket::IPv4, Network::Socket::UDP), _Interrupt(0, 0),
                _Sends(1), _Receives(1), Incomming(1), Outgoing(1), State(States::Stopped), Ready(0, Event::Semaphore)
                {
                    _Socket.Bind(EndPoint);
                }

                inline void Interrupt()
                {
                    _Interrupt.Emit(1);
                }

                inline Descriptor Listener()
                {
                    return Ready.INode();
                }

                bool Take(Request &Request)
                {
                    // Check if we have task in a while loop to make sure

                    Ready.Listen();

                    if (State != States::Running)
                    {
                        Ready.Emit(1);
                        return false;
                    }

                    {
                        std::lock_guard Lock(IncommingLock);

                        if (!Incomming.IsEmpty())
                            Request = Incomming.Take();

                        if (!Incomming.IsEmpty())
                            Ready.Emit(1);
                    }

                    return true;
                }

                bool Put(const Network::EndPoint& Peer, const std::function<void(Format::Serializer&)>& Builder, size_t Size = 1024)
                {
                    if (State != States::Running)
                        return false;

                    // @todo Optimize and generalize this

                    Request request(Peer, Size + 9 /* 9 is default header size */);

                    Format::Serializer Serializer(request.Buffer);

                    Serializer.Add((char *) "CHRD", 4) << (int) 0;

                    Builder(Serializer); // @todo What to do for async calls?

                    Serializer.Modify<uint32_t>(4) = htonl(request.Buffer.Length());

                    {
                        std::lock_guard lock(OutgoingLock);

                        Outgoing.Add(std::move(request));
                    }

                    Interrupt();

                    return true;
                }

                bool Put(Request &&Request)
                {
                    if (State != States::Running)
                        return false;

                    // @todo Optimize this

                    auto len = Request.Buffer.Length();

                    Iterable::Queue<char> Buffer(len + 8, false);

                    // @todo customize identifier

                    Buffer.Add("CHRD\0\0\0\0", 8);

                    *(uint32_t *)(&Buffer[4]) = htonl(len);

                    while (!Request.Buffer.IsEmpty())
                    {
                        Buffer.Add(Request.Buffer.Take());
                    }

                    Request.Buffer = std::move(Buffer);

                    {
                        std::lock_guard lock(OutgoingLock);

                        Outgoing.Add(std::move(Request));
                    }

                    Interrupt();

                    return true;
                }

                void Stop()
                {
                    State = States::Stopped;

                    _Interrupt.Emit(1);

                    Runner.join();

                    std::cout << "Runner stoped" << std::endl;
                }

                void Run()
                {
                    State = States::Running;

                    Runner = std::thread(
                        [this]()
                        {
                            Test::Log("Server") << "Spawning runner" << std::endl;
                            Funtionality();
                        });
                }

                void Funtionality()
                {
                    Iterable::Poll Poll(2);

                    Poll.Add(_Socket.INode(), Network::Socket::In);
                    Poll.Add(_Interrupt.INode(), Network::Socket::In);

                    auto &SocketPoll = Poll[0];
                    auto &InterruptPoll = Poll[1];

                    // @todo Is this thread safe?

                    while (State == States::Running)
                    {
                        // Handle sends if we have data to send

                        if (!_Sends.IsEmpty())
                        {
                            SocketPoll.Set(Iterable::Poll::Out);
                        }
                        else
                        {
                            SocketPoll.Reset(Iterable::Poll::Out);
                        }

                        Poll.Await();

                        // If Socket has event

                        if (SocketPoll.HasEvent())
                        {
                            // If Data arrives

                            if (SocketPoll.Happened(Iterable::Poll::In))
                            {
                                auto len = _Socket.Received();
                                char Data[len];
                                Network::EndPoint Peer;
                                size_t Index;

                                // Read arrived data

                                len = _Socket.ReceiveFrom(Data, len, Peer);

                                // If its a fragmented packet

                                if (_Receives.ContainsWhere(
                                        Index,
                                        [Peer](Request &Item)
                                        {
                                            return !Item.Buffer.IsFull() && Item.Peer == Peer;
                                        }))
                                {
                                    auto &Buffer = _Receives[Index].Buffer;

                                    auto readlen = std::min((Buffer.Capacity() - Buffer.Length()), len);

                                    Buffer.Add(Data, readlen);
                                }
                                else
                                {
                                    // Validate packet header identifier

                                    if (Data[0] != 'C' || Data[1] != 'H' || Data[2] != 'R' || Data[3] != 'D')
                                        continue;

                                    // Get packet length

                                    uint32_t capacity = ntohl(*(uint32_t *)(&Data[4])) - 8;

                                    Iterable::Queue<char> Buffer(capacity, false);

                                    Buffer.Add(&Data[8], len - 8);

                                    Request req;

                                    req.Peer = Peer;
                                    req.Buffer = std::move(Buffer);

                                    _Receives.Add(std::move(req));

                                    Index = _Receives.Length() - 1;

                                    // @todo Fix second request
                                }

                                auto &Current = _Receives[Index];

                                // If its completed move it to Incomming queue

                                if (Current.Buffer.IsFull())
                                {
                                    {
                                        // @todo try lock for non-blocking

                                        std::lock_guard lock(IncommingLock);

                                        Incomming.Add(_Receives.Swap(Index));
                                    }

                                    Ready.Emit(1);
                                }
                            }

                            // If data needs to be sent

                            if (SocketPoll.Happened(Iterable::Poll::Out))
                            {
                                auto &Last = _Sends.Last();

                                // @todo Fix header send

                                Last.Buffer.Free(_Socket.SendTo(Last.Buffer.Content(), Last.Buffer.Length(), Last.Peer));

                                if (Last.Buffer.IsEmpty())
                                    _Sends.Take();
                            }
                        }

                        // If a new data is to be sent

                        if (InterruptPoll.HasEvent())
                        {
                            // @todo try lock for non-blocking

                            std::lock_guard lock(OutgoingLock);

                            while (!Outgoing.IsEmpty())
                            {
                                _Sends.Add(Outgoing.Take());
                            }

                            _Interrupt.Listen();
                        }
                    }

                    // Stopping now

                    Ready.Emit(1);
                }
            };
        }
    }
}
