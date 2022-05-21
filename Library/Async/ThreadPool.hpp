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
                    [this](Entry &Item)
                    {
                        Item.Loop = Async::EventLoop(Interval);
                    });
            }

            ThreadPool(ThreadPool const &Other) = delete;
            ThreadPool &operator=(ThreadPool const &Other) = delete;

            template <typename TCallback>
            void Run(TCallback Condition)
            {
                for (size_t i = 0; i < Loops.Capacity() - 1; ++i)
                {
                    Loops[i].Thread = std::thread(
                        [this, Condition, i]
                        {
                            Loops[i].Loop.Loop(Condition);
                        });
                }
            }

            template <typename TCallback>
            void GetInPool(TCallback Condition)
            {
                if (!Loops.IsFull())
                {
                    Loops.Length(Loops.Capacity());

                    Loops.Last().Loop = Async::EventLoop(Interval);
                }

                Loops.Last().Loop.Loop(Condition);
            }

            void Stop()
            {
                for (size_t i = 0; i < Loops.Capacity() - 1; ++i)
                {
                    Loops[i].Loop.Notify();
                    Loops[i].Thread.join();
                }
            }

            inline size_t Length()
            {
                return Loops.Length();
            }

            inline EventLoop &operator[](size_t Index)
            {
                return Loops[Index].Loop;
            }

        private:
            Iterable::List<Entry> Loops;
            Duration Interval;
        };
    }
}