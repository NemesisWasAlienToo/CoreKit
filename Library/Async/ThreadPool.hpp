#pragma once

#include <string>
#include <mutex>

#include <Event.hpp>
#include <Duration.hpp>
#include <TimeWheel.hpp>
#include <ePoll.hpp>
#include <Iterable/List.hpp>
#include <Network/Socket.hpp>
#include <Network/HTTP/Response.hpp>
#include <Network/HTTP/Request.hpp>
#include <Network/HTTP/Parser.hpp>
#include <Async/EventLoop.hpp>

namespace Core
{
    namespace Async
    {
        class ThreadPool
        {
        public:
            struct Entry
            {
                std::thread Thread;
                EventLoop Loop;
            };

            ThreadPool() = default;
            ThreadPool(Duration const &interval, size_t Count) : Loops(Count + 1, false), Interval(interval)
            {
                Loops.Length(Count);

                Loops.ForEach(
                    [this](EventLoop &Item)
                    {
                        Item = Async::EventLoop(Interval);
                    });
            }

            ThreadPool(ThreadPool const &Other) = delete;
            ThreadPool &operator=(ThreadPool const &Other) = delete;

            template <typename TCallback>
            void Run(TCallback &&Condition)
            {
                for (size_t i = 0; i < Loops.Capacity() - 1; ++i)
                {
                    auto &Loop = Loops[i];

                    Loop.Runner = std::thread(
                        [this, Condition, i]() mutable
                        {
                            auto &Loop = Loops[i];

                            Loop.Loop(std::forward<TCallback>(Condition));
                        });

                    Loop.RunnerId = Loop.Runner.get_id();
                }
            }

            template <typename TCallback>
            void GetInPool(TCallback Condition)
            {
                if (!Loops.IsFull())
                {
                    Loops.Length(Loops.Capacity());

                    Loops.Last() = Async::EventLoop(Interval);
                }

                Loops.Last().RunnerId = std::this_thread::get_id();
                
                Loops.Last().Loop(std::forward<TCallback>(Condition));
            }

            void Stop()
            {
                for (size_t i = 0; i < Loops.Capacity() - 1; ++i)
                {
                    Loops[i].Notify();
                    Loops[i].Runner.join();
                }
            }

            inline size_t Length()
            {
                return Loops.Length();
            }

            // void Enqueue()

            inline EventLoop &operator[](size_t Index)
            {
                return Loops[Index];
            }

        private:
            Iterable::List<EventLoop> Loops;
            Duration Interval;
        };
    }
}