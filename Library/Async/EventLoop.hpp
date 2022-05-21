#pragma once

#include <string>
#include <list>
#include <mutex>

#include <Event.hpp>
#include <Duration.hpp>
#include <TimeWheel.hpp>
#include <ePoll.hpp>
#include <Iterable/Queue.hpp>
#include <Network/Socket.hpp>
#include <Network/HTTP/Response.hpp>
#include <Network/HTTP/Request.hpp>
#include <Network/HTTP/Parser.hpp>

// @todo Safety of assign caller thread

namespace Core
{
    namespace Async
    {
        class EventLoop
        {
        public:
            struct Entry;

            using TimeWheelType = TimeWheel<64, 5>;
            using Container = std::list<Entry>;
            using CallbackType = std::function<void(EventLoop *, ePoll::Entry &, Entry &)>;

            struct Entry
            {
                Descriptor File;
                CallbackType Callback;
                Container::iterator Iterator;

                Iterable::Queue<char> Buffer;
                Network::HTTP::Parser Parser;
                TimeWheelType::Bucket::Iterator Timer;
            };

            EventLoop() = default;
            EventLoop(EventLoop const &Other) = delete;
            EventLoop(EventLoop &&Other) noexcept : _Poll(std::move(Other._Poll)), Expire(std::move(Other.Expire)), Interrupt(std::move(Other.Interrupt)), Wheel(std::move(Other.Wheel)), Handlers(std::move(Other.Handlers)), Queue(std::move(Other.Queue)) {}

            EventLoop(Duration const &Interval) : _Poll(0), Expire(nullptr), Interrupt(nullptr), Wheel(Interval)
            {
                auto IIterator = Insert(
                    Event(0, 0),
                    [](EventLoop *This, ePoll::Entry &Item, Entry &Self)
                    {
                        Event &ev = *static_cast<Event *>(&Self.File);

                        ev.Listen();

                        {
                            std::unique_lock lock(This->QueueMutex);

                            while (!This->Queue.IsEmpty())
                            {
                                auto Des = This->Queue.Take();

                                This->Insert(std::move(Des.File), std::move(Des.Callback));
                            }
                        }
                    });

                // Assgin interrupt event

                Interrupt = static_cast<Event *>(&IIterator->File);

                // Add expire event

                auto TIterator = Insert(
                    Timer(Timer::Monotonic, 0),
                    [](EventLoop *This, auto &Item, auto &Self)
                    {
                        auto &Ev = *static_cast<Event *>(&Self.File);

                        // @todo Fix bug
                        // How does it get here witout event?

                        Ev.Listen();
                        This->Wheel.Tick();
                    });

                // Assign expire event

                Expire = static_cast<Timer *>(&TIterator->File);
            }

            // void Reschedual()

            void Remove(Container::iterator Iterator)
            {
                _Poll.Delete(Iterator->File);
                Handlers.erase(Iterator);
                Wheel.Remove(Iterator->Timer);
            }

            void Modify(Entry &Self, ePoll::Event Events)
            {
                _Poll.Modify(Self.File, Events, (size_t)&Self);
            }

            Container::iterator Assign(Descriptor Client, CallbackType Callback)
            {
                return Insert(std::move(Client), std::move(Callback));
            }

            void Notify(uint64_t Value = 1)
            {
                Interrupt->Emit(Value);
            }

            void Enqueue(Descriptor Client, CallbackType Callback)
            {
                {
                    std::unique_lock lock(QueueMutex);

                    Queue.Add({std::move(Client), std::move(Callback)});
                }

                Interrupt->Emit(1);
            }

            template <typename TCallback>
            void Loop(TCallback Condition)
            {
                auto duration = Wheel.Interval();

                Expire->Set(duration, duration);

                ePoll::List Events(Handlers.size() ? Handlers.size() : 1);

                while (Condition())
                {
                    _Poll(Events);

                    Events.ForEach(
                        [&](ePoll::Entry &Item)
                        {
                            auto &Ent = *Item.DataAs<Entry *>();

                            Ent.Callback(this, Item, Ent);
                        });
                }

                Expire->Stop();
            }

            EventLoop &operator=(EventLoop const &Other) = delete;
            EventLoop &operator=(EventLoop &&Other) noexcept
            {
                if (this == &Other)
                    return *this;

                _Poll = std::move(Other._Poll);
                Expire = std::move(Other.Expire);
                Interrupt = std::move(Other.Interrupt);
                Wheel = std::move(Other.Wheel);
                Handlers = std::move(Other.Handlers);
                Queue = std::move(Other.Queue);

                return *this;
            }

        private:
            Container::iterator Insert(Descriptor descriptor, CallbackType handler)
            {
                auto Iterator = Handlers.insert(Handlers.end(), {std::move(descriptor), std::move(handler)});
                Iterator->Iterator = Iterator;

                // @todo Add timeout timer and remove this

                Iterator->Timer = Wheel.At(0, 0).Entries.end();

                auto &Ent = *Iterator;

                _Poll.Add(Iterator->File, ePoll::In, (size_t)&Ent);

                return Iterator;
            }

            ePoll _Poll;
            Timer *Expire;
            Event *Interrupt;
            TimeWheelType Wheel;
            Container Handlers;

            std::mutex QueueMutex;
            Iterable::Queue<Entry> Queue;
        };
    }
}