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
            using HandlerCallback = std::function<bool(Network::Socket, EndCallback &)>;

            struct Entry
            {
                DateTime Expire;
                EndCallback End;
                HandlerCallback Handler;
            };

            using Map = std::map<Network::EndPoint, Entry>;
            using Queue = Iterable::Queue<Entry>;

            using BuilderCallback = std::function<Entry(const EndPoint &)>;

        protected:
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

            inline void Notify(uint64_t Value = 1)
            {
                Interrupt.Emit(Value);
            }

            inline uint64_t Listen()
            {
                return Interrupt.Listen();
            }

            void SendTo(Entry entry)
            {
                {
                    std::lock_guard Lock(OLock);

                    Outgoing.Add(std::move(entry));
                }

                Interrupt.Emit(1);
            }

            bool ReceiveFrom(const EndPoint &Peer, Entry entry)
            {
                std::lock_guard Lock(ILock);

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

                return true;
            }

            template <typename TCondition>
            void Loop(TCondition Condition)
            {
                while (Condition())
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
                        if (!Condition())
                        {
                            Lock.unlock();
                            break;
                        }

                        Interrupt.Listen();
                        goto calc;
                    }
                }
            }

        protected:
            Core::Network::UDPServer::Map::iterator Soonest()
            {
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
                if (Incomming.size() == 0)
                {
                    Expire.Stop();
                    return;
                }

                Tracking = Soonest();

                SetClock();
            }

            void SetClock()
            {
                if (Tracking->second.Expire.IsExpired())
                {
                    if (Tracking->second.End)
                    {
                        Tracking->second.End();
                    }

                    Wind();

                    return;
                }

                Expire.Set(Tracking->second.Expire.Left());
            }

            bool Clean()
            {
                bool Ret = false;
                Network::EndPoint EP;

                Expire.Listen();
                Lock.unlock();

                {
                    std::lock_guard lock(ILock);

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
                }

                if (Ret && OnClean)
                {
                    OnClean(EP);
                }

                return Ret;
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

                auto IsReady = Last.Handler(Socket, Last.End);

                Lock.unlock();

                if (IsReady)
                {
                    auto Ent = Outgoing.Take();

                    OLock.unlock();

                    Ent.Handler({-1}, Ent.End);
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

                auto IsReady = Iterator->second.Handler(Socket, Iterator->second.End);

                Lock.unlock();

                if (IsReady)
                {
                    auto Ent = std::move(Iterator->second);

                    Incomming.erase(Iterator);

                    Wind();

                    ILock.unlock();

                    Ent.Handler({-1}, Ent.End);
                }

                ILock.unlock();
            }

            inline void Await()
            {
                Poll[0].Mask = Outgoing.IsEmpty() ? (Iterable::Poll::In) : (Iterable::Poll::In | Iterable::Poll::Out);

                Poll();
            }
        };
    }
}