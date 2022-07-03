#pragma once

#include <string>
#include <atomic>
#include <netinet/tcp.h>

#include <Duration.hpp>
#include <Async/ThreadPool.hpp>
#include <Async/Runnable.hpp>

namespace Core
{
    namespace Network
    {
        class TCPServer : public Async::Runnable
        {
        public:
            TCPServer() = default;
            TCPServer(TCPServer const &Other) = delete;

            template <typename TCallback>
            TCPServer(EndPoint const &endPoint, TCallback &&handlerBuilder, Duration const &Timeout, size_t ThreadCount = std::thread::hardware_concurrency(), Duration const &Interval = Duration::FromMilliseconds(500)) : Pool(Interval, ThreadCount)
            {
                Network::Socket Server(static_cast<Network::Socket::SocketFamily>(endPoint.Address().Family()), Network::Socket::TCP);

                // Set Reuse

                Server.SetOptions(SOL_SOCKET, SO_REUSEADDR, static_cast<int>(1));
                Server.SetOptions(SOL_SOCKET, SO_REUSEPORT, static_cast<int>(1));

                // Bind socket

                Server.Bind(endPoint);

                Server.Listen();

                Pool[0].Assign(
                    std::move(Server),
                    [this, HandlerBuilder = std::forward<TCallback>(handlerBuilder), Timeout, Counter = 0](Async::EventLoop *, ePoll::Entry &, Async::EventLoop::Entry &Self) mutable
                    {
                        Network::Socket &Server = *static_cast<Network::Socket *>(&Self.File);

                        auto [Client, Info] = Server.Accept();

                        if (ConnectionCount.fetch_add(1, std::memory_order_relaxed) > Settings.MaxConnectionCount)
                        {
                            ConnectionCount.fetch_sub(1, std::memory_order_relaxed);
                            return;
                        }

                        // Set Non-blocking

                        Client.Blocking(false);

                        // Set NoDelay

                        if (Settings.NoDelay)
                            Client.SetOptions(IPPROTO_TCP, TCP_NODELAY, static_cast<int>(1));

                        auto &Turn = Pool[Counter];

                        Turn.Assign(
                            std::move(Client),
                            HandlerBuilder(Info),
                            [this](Async::EventLoop *, Async::EventLoop::Entry &)
                            {
                                ConnectionCount.fetch_sub(1, std::memory_order_relaxed);
                            },
                            Timeout);

                        Counter = (Counter + 1) % Pool.Length();
                    },
                    nullptr,
                    {0, 0});
            }

            TCPServer &operator=(TCPServer const &Other) = delete;

            Async::ThreadPool& ThreadPool()
            {
                return Pool;
            }

            void Run()
            {
                Async::Runnable::Run();

                Pool.Run(
                    [this]
                    {
                        return Async::Runnable::IsRunning();
                    });
            }

            void GetInPool()
            {
                Pool.GetInPool(
                    [this]
                    {
                        return Async::Runnable::IsRunning();
                    });
            }

            void Stop()
            {
                Async::Runnable::Stop();

                Pool.Stop();
            }

            template <typename TCallback>
            inline void InitStorages(TCallback &&Callback)
            {
                Pool.InitStorages(Callback);
            }

            void MaxConnectionCount(size_t Max)
            {
                Settings.MaxConnectionCount = Max;
            }

            void NoDelay(bool Enable)
            {
                Settings.NoDelay = Enable;
            }

        private:
            Async::ThreadPool Pool;
            std::atomic<size_t> ConnectionCount{0};

            struct
            {
                size_t MaxConnectionCount;
                bool NoDelay;
            } Settings{
                1024,
                false};
        };
    }
}