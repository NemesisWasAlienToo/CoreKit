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
            using BuilderType = std::function<Async::EventLoop::CallbackType()>;

            TCPServer() = default;
            TCPServer(TCPServer const &Other) = delete;
            TCPServer(EndPoint const &endPoint, BuilderType handlerBuilder, Duration const &Timeout, size_t ThreadCount = std::thread::hardware_concurrency(), Duration const &Interval = Duration::FromMilliseconds(500)) : Pool(Interval, ThreadCount), HandlerBuilder(handlerBuilder)
            {
                Network::Socket Server(static_cast<Network::Socket::SocketFamily>(endPoint.Address().Family()), Network::Socket::TCP);

                // Set Reuse

                Server.SetOptions(SOL_SOCKET, SO_REUSEADDR, static_cast<int>(1));
                Server.SetOptions(SOL_SOCKET, SO_REUSEPORT, static_cast<int>(1));

                // Bind socket

                Server.Bind(endPoint);

                Server.Listen();

                // if (Pool.Length() > 0)
                Pool[0].Assign(
                    std::move(Server),
                    [this, Timeout, Counter = 0](Async::EventLoop *, ePoll::Entry &, Async::EventLoop::Entry &Self) mutable
                    {
                        Network::Socket &Server = *static_cast<Network::Socket *>(&Self.File);

                        auto [Client, Info] = Server.Accept();

                        // Set Non-blocking

                        Client.Blocking(false);

                        // Set NoDelay

                        Client.SetOptions(IPPROTO_TCP, TCP_NODELAY, static_cast<int>(1));

                        auto &Turn = Pool[Counter];

                        Turn.Assign(std::move(Client), HandlerBuilder(), Timeout);

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
            BuilderType HandlerBuilder;
        };
    }
}