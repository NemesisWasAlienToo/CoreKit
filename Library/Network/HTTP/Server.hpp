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
    class Server : public Async::Runnable
    {
    public:
        Connection::Settings Settings{
            1024 * 1024 * 1,
            1024 * 1024 * 5,
            1024,
            1024,
            nullptr,
            [this](Connection::Context &Context, Network::HTTP::Request &Request) -> void
            {
                _Router.Match(Request.Path, Request.Method, Context, Request);
            },
            1024,
            false,
            false,
            {5, 0}};

        Server() = default;
        Server(size_t ThreadCount = std::thread::hardware_concurrency(), Duration const &Interval = Duration::FromMilliseconds(500)) : Pool(Interval, ThreadCount), _Router(DefaultRoute)
        {
        }

        // Server functions

        inline Async::ThreadPool &ThreadPool()
        {
            return Pool;
        }

        inline Server &Run()
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

        template <typename TAction>
        inline Server &SetDefault(TAction &&Action)
        {
            _Router.Default = std::forward<TAction>(Action);
            return *this;
        }

        template <ctll::fixed_string TRoute, bool Group = false, typename TAction>
        inline Server &Set(HTTP::Methods Method, TAction &&Action)
        {
            _Router.Add<TRoute, Group>(Method, std::forward<TAction>(Action));
            return *this;
        }

        template <ctll::fixed_string TRoute, bool Group = false, typename TAction>
        inline Server &GET(TAction &&Action)
        {
            return Set<TRoute, Group>(
                HTTP::Methods::GET,
                std::forward<TAction>(Action));
        }

        template <ctll::fixed_string TRoute, bool Group = false, typename TAction>
        inline Server &POST(TAction &&Action)
        {
            return Set<TRoute, Group>(
                HTTP::Methods::POST,
                std::forward<TAction>(Action));
        }

        template <ctll::fixed_string TRoute, bool Group = false, typename TAction>
        inline Server &PUT(TAction &&Action)
        {
            return Set<TRoute, Group>(
                HTTP::Methods::PUT,
                std::forward<TAction>(Action));
        }

        template <ctll::fixed_string TRoute, bool Group = false, typename TAction>
        inline Server &DELETE(TAction &&Action)
        {
            return Set<TRoute, Group>(
                HTTP::Methods::DELETE,
                std::forward<TAction>(Action));
        }

        template <ctll::fixed_string TRoute, bool Group = false, typename TAction>
        inline Server &HEAD(TAction &&Action)
        {
            return Set<TRoute, Group>(
                HTTP::Methods::HEAD,
                std::forward<TAction>(Action));
        }

        template <ctll::fixed_string TRoute, bool Group = false, typename TAction>
        inline Server &OPTIONS(TAction &&Action)
        {
            return Set<TRoute, Group>(
                HTTP::Methods::OPTIONS,
                std::forward<TAction>(Action));
        }

        template <ctll::fixed_string TRoute, bool Group = false, typename TAction>
        inline Server &PATCH(TAction &&Action)
        {
            return Set<TRoute, Group>(
                HTTP::Methods::PATCH,
                std::forward<TAction>(Action));
        }

        template <ctll::fixed_string TRoute, bool Group = false, typename TAction>
        inline Server &Any(TAction &&Action)
        {
            return Set<TRoute, Group>(
                HTTP::Methods::Any,
                std::forward<TAction>(Action));
        }

        template <typename TCallback>
        inline Server &Middleware(TCallback &&Callback)
        {
            using namespace std::placeholders;

            Settings.OnRequest = std::bind(
                std::forward<TCallback>(Callback),
                _1, _2,
                std::move(Settings.OnRequest));

            return *this;
        }

        template <typename TCallback>
        inline Server &Filter(TCallback &&Callback)
        {
            using namespace std::placeholders;

            _Router.Default = std::bind(
                std::forward<TCallback>(Callback),
                _1, _2,
                std::move(_Router.Default));

            return *this;
        }

        template <typename TCallback>
        inline Server &InitStorages(TCallback &&Callback)
        {
            Pool.InitStorages(Callback);
            return *this;
        }

        template <typename TCallback>
        inline Server &OnError(TCallback &&Callback)
        {
            Settings.OnError = std::forward<TCallback>(Callback);
            return *this;
        }

        // Server settings

        inline Server &MaxConnectionCount(size_t Max)
        {
            Settings.MaxConnectionCount = Max;
            return *this;
        }

        inline Server &NoDelay(bool Enable)
        {
            Settings.NoDelay = Enable;
            return *this;
        }

        inline Server &Timeout(Duration const &timeout)
        {
            Settings.Timeout = timeout;
            return *this;
        }

        inline Server &MaxHeaderSize(size_t Size)
        {
            Settings.MaxHeaderSize = Size;
            return *this;
        }

        inline Server &MaxBodySize(size_t Size)
        {
            Settings.MaxBodySize = Size;
            return *this;
        }

        inline Server &RequestBufferSize(size_t Size)
        {
            Settings.RequestBufferSize = Size;
            return *this;
        }

        inline Server &ResponseBufferSize(size_t Size)
        {
            Settings.ResponseBufferSize = Size;
            return *this;
        }

        inline Server &RawContent(bool Value)
        {
            Settings.RawContent = Value;
            return *this;
        }

        template <typename TCallback, typename TEndCallback>
        inline Server &ListenWith(Network::EndPoint const &endPoint, TCallback &&Callback, TEndCallback &&EndCallback)
        {
            Network::Socket Server(static_cast<Network::Socket::SocketFamily>(endPoint.Address().Family()), Network::Socket::TCP);

            // Set Reuse

            Server.SetOptions(SOL_SOCKET, SO_REUSEADDR, static_cast<int>(1));
            Server.SetOptions(SOL_SOCKET, SO_REUSEPORT, static_cast<int>(1));

            // Bind socket

            Server.Bind(endPoint);

            Server.Listen();

            Pool[Turn].Assign(
                std::move(Server),
                std::forward<TCallback>(Callback),
                std::forward<TEndCallback>(EndCallback),
                {0, 0});

            Turn = Pool.Length() ? (Turn + 1) % Pool.Length() : 0;

            return *this;
        }

        inline Server &Listen(Network::EndPoint const &endPoint)
        {
            return ListenWith(
                endPoint,
                [this, endPoint, Counter = 0ull](Async::EventLoop::Context &Context, ePoll::Entry &) mutable
                {
                    Network::Socket &Server = static_cast<Network::Socket &>(Context.Self.File);

                    auto [Client, Info] = Server.Accept();

                    if (!Client)
                        return;

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

                    Pool[Counter].Assign(
                        std::move(Client),
                        Connection(Info, endPoint, Settings),
                        // Async::EventLoop::CallbackType::From<Connection>(Info, endPoint, Settings),
                        [this]
                        {
                            ConnectionCount.fetch_sub(1, std::memory_order_relaxed);
                        },
                        Settings.Timeout);

                    Counter = Pool.Length() ? (Counter + 1) % Pool.Length() : 0;
                },
                nullptr);
        }

        inline Server &Listen(Network::EndPoint const &endPoint, std::string_view Certification, std::string_view Key)
        {
            return ListenWith(
                endPoint,
                [this, endPoint, Counter = 0ull, TLS = TLSContext(Certification, Key)](Async::EventLoop::Context &Context, ePoll::Entry &) mutable
                {
                    Network::Socket &Server = static_cast<Network::Socket &>(Context.Self.File);

                    auto [Client, Info] = Server.Accept();

                    if (!Client)
                        return;

                    if (ConnectionCount.fetch_add(1, std::memory_order_relaxed) > Settings.MaxConnectionCount)
                    {
                        ConnectionCount.fetch_sub(1, std::memory_order_relaxed);
                        return;
                    }

                    // Set Non-blocking

                    Client.Blocking(false);

                    // Set NoDelay

                    if (Settings.NoDelay)
                        Client.SetOptions(IPPROTO_TCP, TCP_NODELAY, 1);

                    // #ifdef TLS_1_2_VERSION
                    // if (true /*Settings.KernelTLS*/)
                    //     Client.SetOptions(SOL_TCP, TCP_ULP, "tls");
                    // #endif

                    auto SS = TLS.NewSocket();

                    SS.SetDescriptor(Client);
                    SS.SetAccept();
                    // SS.SetVerify(SSL_VERIFY_NONE, nullptr);

                    Pool[Counter].Assign(
                        std::move(Client),
                        [this, endPoint, Info = Info, SSL = std::move(SS)](Async::EventLoop::Context &Context, ePoll::Entry &Item) mutable
                        {
                            if (Item.Happened(ePoll::HangUp) || Item.Happened(ePoll::Error))
                            {
                                Context.Remove();
                                return;
                            }

                            //

                            auto Result = SSL.Handshake();

                            if (Result == 1)
                            {
                                SSL.ShakeHand = true;
                                Context.ListenFor(ePoll::In);
                                // Context.Upgrade(Async::EventLoop::CallbackType::From<Connection>(Info, endPoint, Settings, std::move(SSL)), Settings.Timeout);
                                Context.Upgrade(Connection(Info, endPoint, Settings, std::move(SSL)), Settings.Timeout);
                                return;
                            }

                            auto Error = SSL.GetError(Result);

                            if (Error == SSL_ERROR_WANT_WRITE)
                            {
                                Context.ListenFor(ePoll::Out | ePoll::In);
                            }
                            else if (Error == SSL_ERROR_WANT_READ)
                            {
                                Context.ListenFor(ePoll::In);
                            }
                            else
                            {
                                Context.Remove();
                            }
                        },
                        [this]
                        {
                            ConnectionCount.fetch_sub(1, std::memory_order_relaxed);
                        },
                        Settings.Timeout);

                    Counter = Pool.Length() ? (Counter + 1) % Pool.Length() : 0;
                },
                nullptr);
        }

#ifdef __linux__
        inline Server &IgnoreBrokenPipe()
        {
            signal(SIGPIPE, SIG_IGN);
            return *this;
        }
#endif

    private:
        Async::ThreadPool Pool;
        std::atomic<size_t> ConnectionCount{0};
        Router<void(HTTP::Connection::Context &, HTTP::Request &)> _Router;
        volatile size_t Turn = 0;

        static void DefaultRoute(HTTP::Connection::Context &Context, HTTP::Request &Req)
        {
            // @todo Optimize this by saving the response

            Context.SendResponse(HTTP::Response::HTML(Req.Version, HTTP::Status::NotFound, "<h1>404 Not Found</h1>"));
        }
    };
}