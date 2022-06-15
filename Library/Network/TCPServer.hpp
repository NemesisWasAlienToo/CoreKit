#pragma once

#include <string>
#include <netinet/tcp.h>

#include <Duration.hpp>
#include <Async/ThreadPool.hpp>
#include <Async/Runnable.hpp>

namespace Core
{
    namespace Network
    {
        class TCPServer : public Runnable
        {
        public:
            TCPServer() = default;
            TCPServer(TCPServer const &Other) = delete;

            template <typename TCallback>
            TCPServer(EndPoint const &endPoint, TCallback&& handlerBuilder, Duration const &Timeout, size_t ThreadCount = std::thread::hardware_concurrency(), Duration const &Interval = Duration::FromMilliseconds(500)) : Pool(Interval, ThreadCount)
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

                        // Set Non-blocking

                        Client.Blocking(false);

                        // Set NoDelay

                        Client.SetOptions(IPPROTO_TCP, TCP_NODELAY, static_cast<int>(1));

                        auto &Turn = Pool[Counter];

                        Turn.Assign(std::move(Client), HandlerBuilder(Info), Timeout);

                        Counter = (Counter + 1) % Pool.Length();
                    },
                    {0, 0});
            }

            TCPServer &operator=(TCPServer const &Other) = delete;

            void Run()
            {
                Runnable::Run();

                Pool.Run(
                    [this]
                    {
                        return Runnable::IsRunning();
                    });
            }

            void GetInPool()
            {
                Pool.GetInPool(
                    [this]
                    {
                        return Runnable::IsRunning();
                    });
            }

            void Stop()
            {
                Runnable::Stop();

                Pool.Stop();
            }

        private:
            // @todo Since Poll is shared, Make it immutable

            Async::ThreadPool Pool;
        };
    }
}