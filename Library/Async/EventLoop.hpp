#pragma once

#include <string>
#include <list>
#include <thread>
#include <mutex>
#include <memory>

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

            using TimeWheelType = TimeWheel<32, 5>;
            using Container = std::list<Entry>;
            using CallbackType = std::function<void(EventLoop *, ePoll::Entry &, Entry &)>;

            struct Entry
            {
                Descriptor File;
                CallbackType Callback;
                Container::iterator Iterator;

                // @todo Remove Buffer and Parser from here and put it in the callback handler

                TimeWheelType::Bucket::Iterator Timer;
            };

            struct EnqueueEntry
            {
                Descriptor File;
                CallbackType Callback;
                Duration Interval;
            };

            EventLoop() = default;
            EventLoop(EventLoop const &Other) = delete;
            EventLoop(EventLoop &&Other) noexcept : _Poll(std::move(Other._Poll)), Expire(std::move(Other.Expire)), Interrupt(std::move(Other.Interrupt)), Wheel(std::move(Other.Wheel)), Handlers(std::move(Other.Handlers)), Queue(std::move(Other.Queue)) {}

            EventLoop(Duration const &Interval) : _Poll(0), Expire(nullptr), Interrupt(nullptr), Wheel(Interval)
            {
                auto IIterator = Insert(
                    Event(0, 0),
                    [](EventLoop *This, ePoll::Entry &, Entry &Self)
                    {
                        Event &ev = *static_cast<Event *>(&Self.File);

                        ev.Listen();

                        {
                            // @todo Potential buttle neck

                            std::unique_lock lock(This->QueueMutex);

                            while (!This->Queue.IsEmpty())
                            {
                                auto Des = This->Queue.Take();

                                This->Insert(std::move(Des.File), std::move(Des.Callback), std::move(Des.Interval));
                            }
                        }
                    },
                    {0, 0});

                // Assgin interrupt event

                Interrupt = static_cast<Event *>(&IIterator->File);

                // Add expire event

                auto TIterator = Insert(
                    Timer(Timer::Monotonic, 0),
                    [](EventLoop *This, ePoll::Entry &, auto &Self)
                    {
                        auto &Ev = *static_cast<Event *>(&Self.File);

                        Ev.Listen();
                        This->Wheel.Tick();
                    },
                    {0, 0});

                // Assign expire event

                Expire = static_cast<Timer *>(&TIterator->File);
            }

            inline bool HasPermission()
            {
                return RunnerId == std::this_thread::get_id();
            }

            inline void AssertPersmission()
            {
                if (!HasPermission())
                    throw std::runtime_error("Invalid thread");
            }

            void Reschedual(Entry &Self, Duration const &Interval)
            {
                AssertPersmission();

                auto Callback = std::move(Self.Timer->Callback);

                Wheel.Remove(Self.Timer);

                Self.Timer = Wheel.Add(Interval, std::move(Callback));
            }

            void Remove(Container::iterator Iterator)
            {
                AssertPersmission();

                _Poll.Delete(Iterator->File);
                Wheel.Remove(Iterator->Timer);
                Handlers.erase(Iterator);
            }

            /**
             * @brief Only removes the connection handler
             * @param Iterator
             */
            void RemoveHandler(Container::iterator Iterator)
            {
                AssertPersmission();

                _Poll.Delete(Iterator->File);
                Handlers.erase(Iterator);
            }

            void RemoveTimer(Container::iterator Iterator)
            {
                AssertPersmission();

                Wheel.Remove(Iterator->Timer);
            }

            void Modify(Entry &Self, ePoll::Event Events)
            {
                _Poll.Modify(Self.File, Events, (size_t)&Self);
            }

            void Assign(Descriptor &&Client, CallbackType &&Callback, Duration const &Interval = {0, 0})
            {
                if (HasPermission())
                {
                    Insert(std::move(Client), std::move(Callback), Interval);
                }
                else
                {
                    // @todo maybe use lock free queue?

                    {
                        std::unique_lock lock(QueueMutex);

                        if (Client.INode() < 0)
                            throw std::runtime_error("Invalid file descriptor");

                        Queue.Add({std::move(Client), std::move(Callback), Interval});
                    }

                    Notify();
                }
            }

            void Notify(uint64_t Value = 1)
            {
                Interrupt->Emit(Value);
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
                        [this](ePoll::Entry &Item)
                        {
                            auto &Ent = *reinterpret_cast<Entry *>(Item.Data);

                            Ent.Callback(this, Item, Ent);
                        });
                }

                Expire->Stop();
            }

            EventLoop &operator=(EventLoop const &Other) = delete;
            EventLoop &operator=(EventLoop &&Other) noexcept
            {
                if (this != &Other)
                {
                    _Poll = std::move(Other._Poll);
                    Expire = std::move(Other.Expire);
                    Interrupt = std::move(Other.Interrupt);
                    Wheel = std::move(Other.Wheel);
                    Handlers = std::move(Other.Handlers);
                    Queue = std::move(Other.Queue);
                }

                return *this;
            }

        private:
            Container::iterator Insert(Descriptor &&descriptor, CallbackType &&handler, Duration const &Timeout)
            {
                auto Iterator = Handlers.insert(Handlers.end(), {std::move(descriptor), std::move(handler), {}, {}});
                // auto Iterator = Handlers.emplace(std::move(descriptor), std::move(handler), Handlers.end(), Wheel.end());
                Iterator->Iterator = Iterator;

                if (Timeout.AsMilliseconds() > 0)
                {
                    Iterator->Timer = Wheel.Add(
                        Timeout,
                        [this, Iterator]
                        {
                            // Remove(Iterator);

                            /**
                             * @brief Important note
                             * Remove cannot be used here
                             * because this function is called on time out
                             * event in wich an iterator will loop through
                             * the list's entries and clean it.
                             * Calling normal Remove will cause the iterator itseld
                             * to be removed in between iterating through that list
                             * and will cause segmentation fault.
                             * RemoveHandler in turn, will only remove the descriptor
                             * and its handler but not the time-out entry inside our
                             * time wheel object so after executing this callback,
                             * the time wheel handles the task of cleaning the time-out
                             * handler and iterator itself.
                             */
                            RemoveHandler(Iterator);
                        });
                }
                else
                {
                    Iterator->Timer = Wheel.At(0, 0).Entries.end();
                }

                _Poll.Add(Iterator->File, ePoll::In, (size_t) & *Iterator);

                return Iterator;
            }

            ePoll _Poll;
            Timer *Expire;
            Event *Interrupt;
            TimeWheelType Wheel;
            Container Handlers;

            std::mutex QueueMutex;
            Iterable::Queue<EnqueueEntry> Queue;

        public:
            std::thread Runner;
            std::thread::id RunnerId;
            std::shared_ptr<void> Storage = nullptr;
        };
    }
}