#pragma once

#include <string>
#include <list>
#include <thread>
#include <mutex>
#include <memory>
#include <functional>

#include <Event.hpp>
#include <Timer.hpp>
#include <Duration.hpp>
#include <Function.hpp>
#include <TimeWheel.hpp>
#include <ePoll.hpp>
#include <Iterable/Queue.hpp>
#include <Network/Socket.hpp>
#include <Network/HTTP/Response.hpp>
#include <Network/HTTP/Request.hpp>
#include <Network/HTTP/Parser.hpp>

// @todo Safety of assign caller thread

namespace Core::Async
{
    class EventLoop
    {
    public:
        struct Entry;
        struct Context;

        using TimeWheelType = TimeWheel<32, 5>;
        using Container = std::list<Entry>;
        using CallbackType = Core::Function<void(EventLoop::Context &, ePoll::Entry &)>;
        using EndCallbackType = Core::Function<void()>;

        struct Entry
        {
            Descriptor File;
            CallbackType Callback;
            EndCallbackType End;
            Container::iterator Iterator;
            TimeWheelType::Bucket::Iterator Timer;

            template <typename T>
            inline T *CallbackAs()
            {
                return Callback.Target<T>();
            }
        };

        struct Context
        {
            EventLoop &Loop;
            Entry &Self;

            // Context functions

            template <typename T>
            inline T &StorageAs()
            {
                return *std::static_pointer_cast<T>(Loop.Storage);
            }

            template <typename T>
            inline T &HandlerAs() const
            {
                return *Self.CallbackAs<T>();
            }

            template <typename T>
            inline T &FileAs()
            {
                return *static_cast<Network::Socket *>(&Self.File);
            }

            inline void ListenFor(ePoll::Event Events) const
            {
                // Loop.AssertPermission();

                Loop.Modify(Self, Events);
            }

            inline void Remove()
            {
                // Loop.AssertPermission();

                Loop.Remove(Self.Iterator);
            }

            template <typename TCallback>
            inline auto Schedule(Duration const &Timeout, TCallback &&Callback)
            {
                // Loop.AssertPermission();

                return Loop.Schedule(Timeout, std::forward<TCallback>(Callback));
            }

            inline void Reschedule(Duration const &Timeout)
            {
                // Loop.AssertPermission();

                Loop.Reschedule(Self, Timeout);
            }

            inline void Reschedule(Entry &entry, Duration const &Timeout)
            {
                // Loop.AssertPermission();

                Loop.Reschedule(entry, Timeout);
            }

            inline auto Reschedule(TimeWheelType::Bucket::Iterator Iterator, Duration const &Timeout)
            {
                // Loop.AssertPermission();

                return Loop.Reschedule(Iterator, Timeout);
            }

            template <typename TCallback>
            inline void Upgrade(TCallback &&Callback, Duration Timeout, ePoll::Event Events = ePoll::In)
            {
                Loop.Upgrade(Self, std::forward<TCallback>(Callback), Timeout, Events);
            }
        };

        EventLoop() = default;
        EventLoop(EventLoop const &Other) = delete;
        EventLoop(EventLoop &&Other) noexcept : _Poll(std::move(Other._Poll)), Expire(std::move(Other.Expire)), Interrupt(std::move(Other.Interrupt)), Wheel(std::move(Other.Wheel)), Handlers(std::move(Other.Handlers)), Actions(std::move(Other.Actions)) {}

        EventLoop(Duration const &Interval) : _Poll(0), Expire(nullptr), Interrupt(nullptr), Wheel(Interval)
        {
            auto IIterator = Insert(
                Event(0, 0),
                [](EventLoop::Context &Context, ePoll::Entry &)
                {
                    Event &ev = *static_cast<Event *>(&Context.Self.File);

                    ev.Listen();

                    {
                        // @todo Potential bottle neck

                        std::unique_lock lock(Context.Loop.QueueMutex);

                        Context.Loop.Actions.ForEach(
                            [](auto &CB)
                            {
                                CB();
                            });

                        Context.Loop.Actions.Free();
                    }
                },
                nullptr,
                {0, 0});

            // Assign interrupt event

            Interrupt = static_cast<Event *>(&IIterator->File);

            // Add expire event

            auto TIterator = Insert(
                Timer(Timer::Monotonic, 0),
                [](EventLoop::Context &Context, ePoll::Entry &)
                {
                    auto &Ev = *static_cast<Event *>(&Context.Self.File);

                    Ev.Listen();
                    Context.Loop.Wheel.Tick();
                },
                nullptr,
                {0, 0});

            // Assign expire event

            Expire = static_cast<Timer *>(&TIterator->File);
        }

        inline bool HasPermission() const
        {
            return RunnerId == std::this_thread::get_id();
        }

        inline void AssertPermission() const
        {
            if (!HasPermission())
                throw std::runtime_error("Invalid thread");
        }

        template <typename TCallback>
        TimeWheelType::Bucket::Iterator Schedule(Duration const &Interval, TCallback &&Callback)
        {
            AssertPermission();

            return Wheel.Add(Interval, std::forward<TCallback>(Callback));
        }

        TimeWheelType::Bucket::Iterator Reschedule(TimeWheelType::Bucket::Iterator Iterator, Duration const &Interval)
        {
            AssertPermission();

            auto Callback = std::move(Iterator->Callback);

            Wheel.Remove(Iterator);

            return Wheel.Add(Interval, std::move(Callback));
        }

        void Reschedule(Entry &Self, Duration const &Interval)
        {
            Self.Timer = Reschedule(Self.Timer, Interval);
        }

        /**
         * @brief Only removes the connection handler
         * @param Iterator
         */
        void RemoveHandler(Container::iterator Iterator)
        {
            AssertPermission();

            if (Iterator->End)
            {
                Iterator->End();
            }

            _Poll.Delete(Iterator->File);
            Handlers.erase(Iterator);
        }

        void RemoveTimer(Container::iterator Iterator)
        {
            AssertPermission();

            Wheel.Remove(Iterator->Timer);
        }

        void Remove(Container::iterator Iterator)
        {
            Wheel.Remove(Iterator->Timer);
            RemoveHandler(Iterator);
        }

        void Modify(Entry &Self, ePoll::Event Events)
        {
            _Poll.Modify(Self.File, Events, (size_t)&Self);
        }

        template <typename TCallback>
        void Enqueue(TCallback &&Callback)
        {
            {
                using namespace std::placeholders;

                std::unique_lock lock(QueueMutex);

                Actions.Add(std::forward<TCallback>(Callback));
            }

            Notify();
        }

        template <typename TCallback, typename... TArgs>
        void Execute(TCallback &&Callback, TArgs &&...Args)
        {
            if (HasPermission())
            {
                Callback(std::forward<TArgs>(Args)...);
            }
            else
            {
                // @todo maybe use lock free queue?

                {
                    using namespace std::placeholders;

                    std::unique_lock lock(QueueMutex);

                    Actions.Add(
                        [Callback = std::forward<TCallback>(Callback), ... Args = std::forward<TArgs>(Args)]() mutable
                        {
                            Callback(std::forward<TArgs>(Args)...);
                        });
                }

                Notify();
            }
        }

        void Assign(Descriptor &&Client, CallbackType &&Callback, EndCallbackType &&End = nullptr, Duration const &Interval = {0, 0}, ePoll::Event Events = ePoll::In)
        {
            Execute(
                [this, Events](Descriptor &&c, CallbackType &&cb, EndCallbackType &&ecb, Duration const &to) mutable
                {
                    Insert(std::move(c), std::move(cb), std::move(ecb), to, Events);
                },
                std::move(Client), std::move(Callback), std::move(End), Interval);
        }

        void Upgrade(Entry &Self, CallbackType &&Callback, Duration const &Interval = {0, 0}, ePoll::Event Events = ePoll::In)
        {
            Execute(
                [this, Events](Container::iterator si, CallbackType &&cb, Duration const &to) mutable
                {
                    Insert(si, std::move(cb), to, Events);
                },
                Self.Iterator, std::move(Callback), Interval);
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
                        EventLoop::Context Context{*this, *reinterpret_cast<Entry *>(Item.Data)};

                        Context.Self.Callback(Context, Item);
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
                Actions = std::move(Other.Actions);
            }

            return *this;
        }

    private:
        Container::iterator Insert(Descriptor &&descriptor, CallbackType &&handler, EndCallbackType &&end, Duration const &Timeout, ePoll::Event Events = ePoll::In)
        {
            auto Iterator = Handlers.insert(Handlers.end(), {std::move(descriptor), std::move(handler), std::move(end), Handlers.end(), Wheel.end()});
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
                         * event in which an iterator will loop through
                         * the list's entries and clean it.
                         * Calling normal Remove will cause the iterator itself
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

            _Poll.Add(Iterator->File, Events, (size_t) & *Iterator);

            return Iterator;
        }

        Container::iterator Insert(Container::iterator Item, CallbackType &&handler, Duration const &Timeout, ePoll::Event Events = ePoll::In)
        {
            auto Iterator = Handlers.insert(Handlers.end(), {std::move(Item->File), std::move(handler), std::move(Item->End), Handlers.end(), Wheel.end()});
            Iterator->Iterator = Iterator;

            if (Timeout.AsMilliseconds() > 0)
            {
                Iterator->Timer = Wheel.Add(
                    Timeout,
                    [this, Iterator]
                    {
                        RemoveHandler(Iterator);
                    });
            }
            else
            {
                Iterator->Timer = Wheel.At(0, 0).Entries.end();
            }

            _Poll.Modify(Iterator->File, Events, (size_t) & *Iterator);

            RemoveTimer(Item);
            Handlers.erase(Item);

            return Iterator;
        }

        ePoll _Poll;
        Timer *Expire;
        Event *Interrupt;
        TimeWheelType Wheel;
        Container Handlers;

        std::mutex QueueMutex;
        Iterable::Queue<Core::Function<void()>> Actions;

    public:
        std::thread Runner;
        std::thread::id RunnerId;
        std::shared_ptr<void> Storage = nullptr;
    };
}