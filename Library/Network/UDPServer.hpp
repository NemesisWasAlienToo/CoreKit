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

#include "Test.hpp"
#include "Event.hpp"
#include "Timer.hpp"

#include "Network/EndPoint.hpp"
#include "Network/Socket.hpp"

#include "Iterable/List.hpp"
#include "Iterable/Poll.hpp"

#include <Format/Serializer.hpp>
#include "Network/DHT/DHT.hpp"

namespace Core
{
    namespace Network
    {
        class UDPServer
        {
        public:
            using CleanCallback = std::function<void(const EndPoint &)>;
            using ProductCallback = std::function<void()>;

            using EndCallback = std::function<void()>;
            using HandlerCallback = std::function<std::tuple<bool, ProductCallback>(const Network::Socket &, EndCallback &)>;

            struct Entry
            {
                DateTime Expire;
                EndCallback End;
                HandlerCallback Handler;
            };

            using Map = std::map<Network::EndPoint, Entry>;
            using Queue = Iterable::Queue<Entry>;

            using BuilderCallback = std::function<Entry(const EndPoint &)>;

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

            void SendTo(Entry entry)
            {
                OLock.lock();

                Outgoing.Add(std::move(entry));

                OLock.unlock();

                Interrupt.Emit(1);
            }

            bool ReceiveFrom(const EndPoint &Peer, Entry entry)
            {
                ILock.lock();

                auto [Iterator, Inserted] = Incomming.try_emplace(Peer, Entry());

                if (!Inserted)
                {
                    return false;
                }

                Iterator->second = std::move(entry);

                // Optimize winding

                if (Incomming.size() == 1 || Iterator->second.Expire < Tracking->second.Expire)
                {
                    Tracking = Iterator;
                }

                SetClock();

                ILock.unlock();

                return true;
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

                auto [Finished, Product] = Last.Handler(Socket, Last.End);

                Lock.unlock();

                if (Finished)
                {
                    Outgoing.Take();

                    OLock.unlock();

                    if (Product)
                    {
                        Product();
                    }
                }
            }

            void OnReceive()
            {
                EndPoint Peer;
                Socket.ReceiveFrom(nullptr, 0, Peer, Network::Socket::Peek);

                ILock.lock();

                auto [Iterator, Inserted] = Incomming.try_emplace(Peer, Entry());

                if (Inserted)
                {
                    // @todo if Data contains multiple packets, Builder must add multiple handelrs

                    Iterator->second = Builder(Peer);
                }

                auto [Ended, Product] = Iterator->second.Handler(Socket, Iterator->second.End);

                Lock.unlock();

                if (Ended)
                {
                    Incomming.erase(Iterator);

                    Wind();

                    ILock.unlock();

                    if (Product)
                    {
                        Product();
                    }
                }

                ILock.unlock();
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
                        Left = Duration(0, 0);
                    }

                    Expire.Set(Left);
                }
                else
                {
                    Expire.Stop();
                }
            }

            void SetClock()
            {
                Duration Left;

                if (Tracking->second.Expire > DateTime::Now())
                {
                    Left = Tracking->second.Expire.Left();
                }
                else
                {
                    Left = Duration(0, 0);
                }

                Expire.Set(Left);
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
                Await();

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