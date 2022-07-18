#pragma once

#include <string>
#include <list>
#include <thread>
#include <mutex>
#include <memory>
#include <functional>

#include <Event.hpp>
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

struct ThrowOnCopy
{
    ThrowOnCopy() = default;
    ThrowOnCopy(const ThrowOnCopy &) { throw std::logic_error("Oops!"); }
    ThrowOnCopy(ThrowOnCopy &&) = default;
    ThrowOnCopy &operator=(ThrowOnCopy &&) = default;
};

template <typename T>
struct FakeCopyable : ThrowOnCopy
{
    FakeCopyable(T &&t) : target(std::forward<T>(t)) {}

    FakeCopyable(FakeCopyable &&) = default;

    FakeCopyable(const FakeCopyable &other)
        : ThrowOnCopy(other),                              // this will throw
          target(std::move(const_cast<T &>(other.target))) // never reached
    {
    }

    template <typename... Args>
    auto operator()(Args &&...a)
    {
        return target(std::forward<Args>(a)...);
    }

    T target;
};

template <typename T>
FakeCopyable<T>
fake_copyable(T &&t)
{
    return {std::forward<T>(t)};
}

namespace Core::Async
{
    class EventLoop
    {
    public:
        struct Entry;
        struct Context;

        using TimeWheelType = TimeWheel<32, 5>;
        using Container = std::list<Entry>;
        using CallbackType = std::function<void(EventLoop::Context &, ePoll::Entry &)>;
        using EndCallbackType = std::function<void()>;

        struct Entry
        {
            Descriptor File;
            CallbackType Callback;
            EndCallbackType End;
            Container::iterator Iterator;
            TimeWheelType::Bucket::Iterator Timer;

            template <typename T>
            T *CallbackAs()
            {
                // return Callback.Target<T>();

                return Callback.target<T>();
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
            inline T &HandlerAs()
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
                // Loop.AssertPersmission();

                Loop.Modify(Self, Events);
            }

            inline void Remove()
            {
                // Loop.AssertPersmission();

                Loop.Remove(Self.Iterator);
            }

            template <typename TCallback>
            inline auto Schedual(Duration const &Timeout, TCallback &&Callback)
            {
                // Loop.AssertPersmission();

                return Loop.Schedual(Timeout, std::forward<TCallback>(Callback));
            }

            inline void Reschedual(Duration const &Timeout)
            {
                // Loop.AssertPersmission();

                Loop.Reschedual(Self, Timeout);
            }

            inline void Reschedual(Entry &entry, Duration const &Timeout)
            {
                // Loop.AssertPersmission();

                Loop.Reschedual(entry, Timeout);
            }

            inline auto Reschedual(TimeWheelType::Bucket::Iterator Iterator, Duration const &Timeout)
            {
                // Loop.AssertPersmission();

                return Loop.Reschedual(Iterator, Timeout);
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
                        // @todo Potential buttle neck

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

            // Assgin interrupt event

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

        inline void AssertPersmission() const
        {
            if (!HasPermission())
                throw std::runtime_error("Invalid thread");
        }

        template <typename TCallback>
        TimeWheelType::Bucket::Iterator Schedual(Duration const &Interval, TCallback &&Callback)
        {
            AssertPersmission();

            return Wheel.Add(Interval, std::forward<TCallback>(Callback));
        }

        TimeWheelType::Bucket::Iterator Reschedual(TimeWheelType::Bucket::Iterator Iterator, Duration const &Interval)
        {
            AssertPersmission();

            auto Callback = std::move(Iterator->Callback);

            Wheel.Remove(Iterator);

            return Wheel.Add(Interval, std::move(Callback));
        }

        void Reschedual(Entry &Self, Duration const &Interval)
        {
            Self.Timer = Reschedual(Self.Timer, Interval);
        }

        /**
         * @brief Only removes the connection handler
         * @param Iterator
         */
        void RemoveHandler(Container::iterator Iterator)
        {
            AssertPersmission();

            if (Iterator->End)
            {
                Iterator->End();
            }

            _Poll.Delete(Iterator->File);
            Handlers.erase(Iterator);
        }

        void RemoveTimer(Container::iterator Iterator)
        {
            AssertPersmission();

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

                    // Actions.Add(std::bind(
                    //     std::forward<TCallback>(Callback),
                    //     _1,
                    //     std::forward<TArgs>(Args)...));

                    Actions.Add(fake_copyable(
                        [Callback = std::forward<TCallback>(Callback), ... Args = std::forward<TArgs>(Args)]() mutable
                        {
                            Callback(std::forward<TArgs>(Args)...);
                        }));
                }

                Notify();
            }
        }

        void Assign(Descriptor &&Client, CallbackType &&Callback, EndCallbackType &&End = nullptr, Duration const &Interval = {0, 0})
        {
            Execute(
                [this](Descriptor &&c, CallbackType &&cb, EndCallbackType &&ecb, Duration const &to) mutable
                {
                    Insert(std::move(c), std::move(cb), std::move(ecb), to);
                },
                std::move(Client), std::move(Callback), std::move(End), Interval);
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
        Container::iterator Insert(Descriptor &&descriptor, CallbackType &&handler, EndCallbackType &&end, Duration const &Timeout)
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
        Iterable::Queue<std::function<void()>> Actions;

    public:
        std::thread Runner;
        std::thread::id RunnerId;
        std::shared_ptr<void> Storage = nullptr;
    };
}