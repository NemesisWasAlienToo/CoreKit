#pragma once

#include <string>
#include <optional>
#include <signal.h>
#include <netinet/tcp.h>

#include <Duration.hpp>
#include <Network/Socket.hpp>
#include <Format/Stream.hpp>
#include <Network/HTTP/Response.hpp>
#include <Network/HTTP/Request.hpp>
#include <Async/ThreadPool.hpp>
#include <Network/HTTP/Router.hpp>
#include <Network/HTTP/Connection.hpp>
#include <Async/Runnable.hpp>

namespace Core::Network::HTTP
{
    template <template <typename> typename... TModules>
    class Server : public Async::Runnable, public TModules<Server<TModules...>>...
    {
    public:
        Server(size_t ThreadCount = std::thread::hardware_concurrency(), Duration const &Interval = Duration::FromMilliseconds(500)) : Pool(Interval, ThreadCount)

        {
        }

        inline Async::ThreadPool &ThreadPool()
        {
            return Pool;
        }

        inline auto &ConnectionCountAtomic()
        {
            return ConnectionCount;
        }

        inline auto &Run()
        {
            Async::Runnable::Run();

            Pool.Run(
                [this]
                {
                    return Async::Runnable::IsRunning();
                });

            return *this;
        }

        inline void GetInPool()
        {
            Pool.GetInPool(
                [this]
                {
                    return Async::Runnable::IsRunning();
                });
        }

        inline void Stop()
        {
            Async::Runnable::Stop();

            Pool.Stop();
        }

        template <typename TCallback, typename TEndCallback>
        inline auto &ListenWith(Network::EndPoint const &endPoint, TCallback &&Callback, TEndCallback &&EndCallback)
        {
            Network::Socket Router(static_cast<Network::Socket::SocketFamily>(endPoint.Address().Family()), Network::Socket::TCP);

            // Set Reuse

            Router.SetOptions(SOL_SOCKET, SO_REUSEADDR, static_cast<int>(1));
            Router.SetOptions(SOL_SOCKET, SO_REUSEPORT, static_cast<int>(1));

            // Bind socket

            Router.Bind(endPoint);

            Router.Listen();

            Pool[Turn].Assign(
                std::move(Router),
                std::forward<TCallback>(Callback),
                std::forward<TEndCallback>(EndCallback),
                {0, 0});

            Turn = Pool.Length() ? (Turn + 1) % Pool.Length() : 0;

            return *this;
        }

        inline bool TryIncrementConnectionCount()
        {
            if (ConnectionCount.fetch_add(1, std::memory_order_relaxed) > MaxConnectionCount)
            {
                ConnectionCount.fetch_sub(1, std::memory_order_relaxed);
                return false;
            }

            return true;
        }

        inline void DecrementConnectionCount()
        {
            ConnectionCount.fetch_sub(1, std::memory_order_relaxed);
        }

        inline auto &MaxConnections(size_t Count)
        {
            MaxConnectionCount = Count;
            return *this;
        }

#ifdef __linux__
        inline auto &IgnoreBrokenPipe()
        {
            signal(SIGPIPE, SIG_IGN);
            return *this;
        }
#endif

    protected:
        size_t MaxConnectionCount{1024};
        std::atomic<size_t> ConnectionCount{0};
        Async::ThreadPool Pool;
        volatile size_t Turn = 0;
    };
}