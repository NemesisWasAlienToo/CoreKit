/**
 * @file UDPServer.cpp
 * @author Nemesis (github.com/NemesisWasAlienToo)
 * @brief
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

#include "Test.cpp"
#include "Event.cpp"
#include "Timer.cpp"

#include "Network/EndPoint.cpp"
#include "Network/Socket.cpp"

#include "Iterable/List.cpp"
#include "Iterable/Poll.cpp"

#include <Format/Serializer.cpp>
#include "Network/DHT/DHT.cpp"

namespace Core
{
    namespace Network
    {
        class UDPServer
        {
        public:
            using CleanCallback = std::function<void(const EndPoint &)>;

            using EndCallback = std::function<void()>;
            using OutgoingCallback = std::function<bool(const Network::Socket &)>;

            using ProductCallback = std::function<void()>;

            using IncommingCallback = std::function<std::tuple<bool, ProductCallback>(const Network::Socket &, EndCallback &)>;

            struct OutEntry
            {
                EndCallback End;
                OutgoingCallback Handler;
            };

            struct InEntry
            {
                DateTime Expire;
                EndCallback End;
                IncommingCallback Handler;
            };

            using Map = std::map<Network::EndPoint, InEntry>;
            using Queue = Iterable::Queue<OutEntry>;

            using BuilderCallback = std::function<void(Map::iterator &)>;

        private:
            std::mutex Lock;

            Timer Expire;
            Event Interrupt;
            Network::Socket Socket;

            Iterable::Poll Poll;

            Duration TimeOut;

            std::mutex OLock;
            Queue Outgoing;

            std::mutex ILock;
            Map Incomming;

            Map::iterator Tracking;

        public:
            BuilderCallback Builder;
            CleanCallback OnClean;

            // Constructors

            UDPServer() = default;

            UDPServer(const Network::EndPoint &EndPoint) : Expire(Timer::Monotonic, 0), Interrupt(0, 0), Socket(Network::Socket::IPv4, Network::Socket::UDP), Poll(3)
            {
                Socket.Bind(EndPoint);

                Poll.Add(Socket, Network::Socket::In);
                Poll.Add(Expire, Timer::In);
                Poll.Add(Interrupt, Event::In);
            }

            // Functionalities

            // @todo Make this callback based like ReceiveFrom

            void SendTo(OutgoingCallback Handler, EndCallback End)
            {
                OLock.lock();

                Outgoing.Add({std::move(End), std::move(Handler)});

                OLock.unlock();

                Interrupt.Emit(1);
            }

            template<class TCallback>
            void SendTo(const TCallback& Callback)
            {
                OLock.lock();

                Outgoing.Add({nullptr, nullptr});

                // @todo Fix : Due to reallocation, Address might change

                Callback(Outgoing.Length() - 1);

                OLock.unlock();

                Interrupt.Emit(1);
            }

            template<class TCallback>
            bool ReceiveFrom(const EndPoint& Peer, const TCallback& Callback)
            {
                ILock.lock();

                // @todo Fix : Due to reallocation, Address might change

                auto [Iterator, Inserted] = Incomming.try_emplace(Peer, InEntry());

                Callback(Iterator, Inserted);

                Wind();

                ILock.unlock();

                return Inserted;
            }

            void OnSend()
            {
                // @todo unclock mutex on exception

                OLock.lock();

                if (Outgoing.IsEmpty())
                {
                    OLock.unlock();
                    return;
                }

                auto &Last = Outgoing.Last();

                if (Last.Handler(Socket))
                {
                    Lock.unlock();

                    OutEntry entry = std::move(Last);

                    Outgoing.Take();

                    // if(Outgoing.IsEmpty())
                    // {
                    //     Empty.Emit(1);
                    // }

                    OLock.unlock();

                    if (entry.End)
                    {
                        entry.End();
                    }
                }
            }

            void OnReceive()
            {
                EndPoint Peer;
                Socket.ReceiveFrom(nullptr, 0, Peer, Network::Socket::Peek);

                ILock.lock();

                auto [Entry, Inserted] = Incomming.try_emplace(Peer, InEntry());

                if (Inserted)
                {
                    // @todo if Data contains multiple packets, Builder must add multiple handelrs

                    Builder(Entry);
                }

                auto [Ended, Product] = Entry->second.Handler(Socket, Entry->second.End);

                Lock.unlock();
                ILock.unlock();

                if (Ended)
                {
                    Incomming.erase(Entry);

                    ILock.lock();

                    Wind();

                    ILock.unlock();

                    if (Product)
                    {
                        Product();
                    }
                }
            }

            auto Soonest()
            {
                if (Incomming.size() <= 0)
                    throw std::out_of_range("Instance contains no handler");

                auto Result = Incomming.begin();
                auto It = Incomming.begin()++;

                for (size_t i = 1; i < Incomming.size(); i++, It++)
                {
                    if (It->second.Expire < Result->second.Expire)
                    {
                        Result = It;
                    }
                }

                return Result;
            }

            void Wind()
            {
                if (Incomming.size() != 0)
                {
                    Tracking = Soonest();

                    Duration Left;

                    if (Tracking->second.Expire > DateTime::Now())
                    {
                        Left = Tracking->second.Expire.Left();
                    }
                    else
                    {
                        Left = Duration(0, 1);
                    }

                    Expire.Set(Left);
                }
                else
                {
                    // Tracking->second.Expire = DateTime::Now();
                    Expire.Stop();
                }
            }

            bool Clean() // @todo Add OnClean as callback template
            {
                bool Ret = false;
                Network::EndPoint EP;

                Expire.Value();

                Lock.unlock();

                {
                    ILock.lock();

                    if ((Ret = Tracking->second.Expire.IsExpired()))
                    {
                        if (Tracking->second.End)
                        {
                            Tracking->second.End();
                        }

                        Incomming.erase(Tracking);
                        EP = Tracking->first;
                    }

                    Wind();

                    ILock.unlock();
                }

                if (Ret && OnClean)
                {
                    OnClean(EP);
                }

                return Ret;
            }

            inline void Await()
            {
                Poll[0].Mask = Outgoing.IsEmpty() ? (Iterable::Poll::In) : (Iterable::Poll::In | Iterable::Poll::Out);

                Poll();
            }

            void Loop()
            {
                Lock.lock();

            calc:
                Await(); // <- Locks the poll

                if (Poll[0].HasEvent())
                {
                    if (Poll[0].Happened(Iterable::Poll::Out))
                    {
                        OnSend();
                    }
                    else if (Poll[0].Happened(Iterable::Poll::In))
                    {
                        OnReceive();
                    }
                }
                else if (Poll[1].HasEvent())
                {
                    Clean();
                }
                else if (Poll[2].HasEvent())
                {
                    Interrupt.Listen();
                    goto calc;
                }
            }
        };
    }
}