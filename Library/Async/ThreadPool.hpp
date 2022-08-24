#pragma once

#include <string>
#include <mutex>
#include <future>
#include <atomic>
#include <mutex>

#include <Event.hpp>
#include <Duration.hpp>
#include <TimeWheel.hpp>
#include <ePoll.hpp>
#include <Iterable/Span.hpp>
#include <Network/Socket.hpp>
#include <Network/HTTP/Response.hpp>
#include <Network/HTTP/Request.hpp>
#include <Network/HTTP/Parser.hpp>
#include <Async/EventLoop.hpp>

namespace Core::Async
{
    class ThreadPool
    {
    public:
        ThreadPool() = default;
        ThreadPool(Duration const &interval, size_t Count) : Loops(Count + 1), Interval(interval)
        {
            Loops.ForEach(
                [this](EventLoop &Item)
                {
                    Item = Async::EventLoop(Interval);
                });

            Loops.Last().RunnerId = std::this_thread::get_id();
        }

        ThreadPool(ThreadPool const &Other) = delete;
        ThreadPool &operator=(ThreadPool const &Other) = delete;

        template <typename TCallback>
        void Run(TCallback &&Condition)
        {
            for (size_t i = 0; i < Loops.Length() - 1; ++i)
            {
                auto &Loop = Loops[i];

                Loop.Runner = std::thread(
                    [this, Condition, i]() mutable
                    {
                        auto &Loop = Loops[i];

                        AwaitGo();

                        Loop.Loop(std::forward<TCallback>(Condition));
                    });

                Loop.RunnerId = Loop.Runner.get_id();
            }

            SignalGo();
        }

        template <typename TCallback>
        void GetInPool(TCallback Condition)
        {
            HasJoined.store(true);

            AwaitGo();

            Loops.Last().Loop(std::forward<TCallback>(Condition));

            HasJoined.store(false);
        }

        void Stop()
        {
            for (size_t i = 0; i < Loops.Length() - 1; ++i)
            {
                Loops[i].Notify();
                Loops[i].Runner.join();
            }
        }

        inline size_t Length()
        {
            // @todo Potential bottle neck

            return HasJoined.load(std::memory_order_relaxed) ? Loops.Length() : Loops.Length() - 1;
        }

        inline EventLoop &operator[](size_t Index)
        {
            return Loops[Index];
        }

        template <typename TCallback>
        inline void InitStorages(TCallback &&Callback)
        {
            for (size_t i = 0; i < Loops.Length(); i++)
            {
                Callback(Loops[i].Storage);
            }
        }

    private:
        Iterable::Span<EventLoop> Loops;
        Duration Interval;
        std::atomic_bool HasJoined{false};

        std::promise<void> GoPromise;
        std::shared_future<void> GoFuture{GoPromise.get_future()};

        void SignalGo()
        {
            GoPromise.set_value();
        }

        void AwaitGo()
        {
            GoFuture.get();
        }
    };
}