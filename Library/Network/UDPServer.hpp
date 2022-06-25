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
#include <unordered_map>

#include "Poll.hpp"
#include "Test.hpp"
#include "Event.hpp"
#include "Timer.hpp"
#include "TimeWheel.hpp"

#include "Network/EndPoint.hpp"
#include "Network/Socket.hpp"

#include "Iterable/List.hpp"

#include <Format/Serializer.hpp>
#include "Network/DHT/DHT.hpp"

namespace Core
{
    namespace Network
    {
        class UDPServer
        {
        public:
            using WheelType = TimeWheel<20, 10>;
            using CleanCallback = std::function<void(const EndPoint &)>;
            using ProductCallback = std::function<void()>;

            using EndCallback = std::function<void()>;
            using HandlerCallback = std::function<bool(Network::Socket const &, EndCallback &)>;

            struct Entry
            {
                EndCallback End;
                HandlerCallback Handler;
                WheelType::Bucket::Iterator Timer;
            };

            using Map = std::unordered_map<Network::EndPoint, Entry>;
            using Queue = Iterable::Queue<Entry>;

            using BuilderCallback = std::function<Entry(const EndPoint &)>;

        protected:
            std::mutex Lock;

            Duration TimeOut;
            Timer Expire;
            WheelType Wheel;
            Event Interrupt;
            Network::Socket Socket;

            Core::Poll Poll;
            Iterable::List<Core::Poll::Entry> Events;

            std::mutex OLock;
            Queue Outgoing;

            std::mutex ILock;
            Map Incomming;

            // @todo implement this

            size_t MaxConnection = 0;

        public:
            BuilderCallback Builder;
            CleanCallback OnClean;

            // Constructors

            UDPServer() = default;

            UDPServer(const Network::EndPoint &EndPoint, Duration const &Timeout, Duration const &Interval) : TimeOut(Timeout), Expire(Timer::Monotonic, 0), Wheel(Interval), Interrupt(0, 0), Socket(Network::Socket::IPv4, Network::Socket::UDP)
            {
                Socket.Bind(EndPoint);

                Events.Add(Socket, Core::Poll::In);
                Events.Add(Expire, Core::Poll::In);
                Events.Add(Interrupt, Core::Poll::In);

                // @todo Should it be here?

                Expire.Set(Interval, Interval);
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

                auto Ptr = Wheel.Add(
                    TimeOut,
                    [this, Iterator]
                    {
                        std::lock_guard Lock(ILock);

                        if (Iterator->second.End)
                        {
                            Iterator->second.End();
                        }

                        auto EP = Iterator->first;

                        Incomming.erase(Iterator);

                        OnClean(EP);
                    });

                Iterator->second.Timer = Ptr;

                return true;
            }

            template <typename TCondition>
            void Loop(TCondition Condition)
            {
                while (Condition())
                {
                    std::unique_lock _Lock(Lock);

                calc:
                    Await();

                    if (Events[0].HasEvent())
                    {
                        if (Events[0].Happened(Core::Poll::Out))
                        {
                            OnSend();
                        }
                        else if (Events[0].Happened(Core::Poll::In))
                        {
                            OnReceive();
                        }
                    }
                    else if (Events[1].HasEvent())
                    {
                        Expire.Listen();
                        Wheel.Tick();
                    }
                    else if (Events[2].HasEvent())
                    {
                        if (!Condition())
                        {
                            _Lock.unlock();
                            break;
                        }

                        Interrupt.Listen();
                        goto calc;
                    }
                }
            }

        protected:
            void OnSend()
            {
                // @todo unclock mutex on exception

                OLock.lock();

                if (Outgoing.IsEmpty())
                {
                    OLock.unlock();
                    return;
                }

                auto &Last = Outgoing.Tail();

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

                {
                    std::unique_lock _lock(ILock);

                    auto [Iterator, Inserted] = Incomming.try_emplace(Peer, Entry());

                    if (Inserted)
                    {
                        // @todo if Data contains multiple packets, Builder must add multiple handelrs

                        Iterator->second = Builder(Peer);

                        auto Ptr = Wheel.Add(
                            TimeOut,
                            [this, Iterator]
                            {
                                std::lock_guard Lock(ILock);

                                if (Iterator->second.End)
                                {
                                    Iterator->second.End();
                                }

                                auto EP = Iterator->first;

                                Incomming.erase(Iterator);

                                OnClean(EP);
                            });

                        Iterator->second.Timer = Ptr;
                    }

                    auto IsReady = Iterator->second.Handler(Socket, Iterator->second.End);

                    Lock.unlock();

                    if (IsReady)
                    {
                        // @todo Check for concurrent access

                        Wheel.Remove(Iterator->second.Timer);

                        auto Ent = std::move(Iterator->second);

                        Incomming.erase(Iterator);

                        ILock.unlock();

                        Ent.Handler({}, Ent.End);
                    }
                }
            }

            inline void Await()
            {
                Events[0].Mask = Outgoing.IsEmpty() ? (Core::Poll::In) : (Core::Poll::In | Core::Poll::Out);

                Poll(Events);
            }
        };
    }
}