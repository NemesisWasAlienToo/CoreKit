#pragma once

#include <netinet/tcp.h>

#include <Duration.hpp>
#include <Network/Socket.hpp>
#include <Format/Stream.hpp>
#include <Network/HTTP/Response.hpp>
#include <Network/HTTP/Request.hpp>
#include <Async/ThreadPool.hpp>
#include <Network/HTTP/Router.hpp>
#include <Network/HTTP/Connection.hpp>

namespace Core::Network::HTTP::Modules
{
    template <typename T>
    class Router
    {
    public:
        Router() : _Router(DefaultRoute) {}

        // Router functions

        template <typename TAction>
        inline T &SetDefault(TAction &&Action)
        {
            _Router.Default = std::forward<TAction>(Action);
            return static_cast<T &>(*this);
        }

        template <ctll::fixed_string TRoute, typename TAction>
        inline T &Set(HTTP::Methods Method, TAction &&Action)
        {
            static_assert(TRoute.size() > 0 && TRoute[0] == '/', "Route must start with \'/\' character");
            static_assert((TRoute.size() == 1 && TRoute[0] == '/') || (TRoute[TRoute.size() - 1] != '/'), "Route must start with \'/\' character");

            _Router.Add<TRoute>(Method, std::forward<TAction>(Action));
            return static_cast<T &>(*this);
        }

        template <ctll::fixed_string TRoute, typename TAction>
        inline auto &GET(TAction &&Action)
        {
            return Set<TRoute>(
                HTTP::Methods::GET,
                std::forward<TAction>(Action));
        }

        template <ctll::fixed_string TRoute, typename TAction>
        inline auto &POST(TAction &&Action)
        {
            return Set<TRoute>(
                HTTP::Methods::POST,
                std::forward<TAction>(Action));
        }

        template <ctll::fixed_string TRoute, typename TAction>
        inline auto &PUT(TAction &&Action)
        {
            return Set<TRoute>(
                HTTP::Methods::PUT,
                std::forward<TAction>(Action));
        }

        template <ctll::fixed_string TRoute, typename TAction>
        inline auto &DELETE(TAction &&Action)
        {
            return Set<TRoute>(
                HTTP::Methods::DELETE,
                std::forward<TAction>(Action));
        }

        template <ctll::fixed_string TRoute, typename TAction>
        inline auto &HEAD(TAction &&Action)
        {
            return Set<TRoute>(
                HTTP::Methods::HEAD,
                std::forward<TAction>(Action));
        }

        template <ctll::fixed_string TRoute, typename TAction>
        inline auto &OPTIONS(TAction &&Action)
        {
            return Set<TRoute>(
                HTTP::Methods::OPTIONS,
                std::forward<TAction>(Action));
        }

        template <ctll::fixed_string TRoute, typename TAction>
        inline auto &PATCH(TAction &&Action)
        {
            return Set<TRoute>(
                HTTP::Methods::PATCH,
                std::forward<TAction>(Action));
        }

        template <ctll::fixed_string TRoute, typename TAction>
        inline auto &Any(TAction &&Action)
        {
            return Set<TRoute>(
                HTTP::Methods::Any,
                std::forward<TAction>(Action));
        }

        template <typename TCallback>
        inline T &Processor(TCallback &&Callback)
        {
            using namespace std::placeholders;

            Settings.OnResponse = std::bind(
                std::forward<TCallback>(Callback),
                _1, _2, _3, _4,
                std::move(Settings.OnResponse));

            return static_cast<T &>(*this);
        }

        template <typename TCallback>
        inline T &Middleware(TCallback &&Callback)
        {
            using namespace std::placeholders;

            Settings.OnRequest = std::bind(
                std::forward<TCallback>(Callback),
                _1, _2,
                std::move(Settings.OnRequest));

            return static_cast<T &>(*this);
        }

        template <typename TCallback>
        inline T &Filter(TCallback &&Callback)
        {
            using namespace std::placeholders;

            _Router.Default = std::bind(
                std::forward<TCallback>(Callback),
                _1, _2,
                std::move(_Router.Default));

            return static_cast<T &>(*this);
        }

        template <typename TCallback>
        inline T &InitStorages(TCallback &&Callback)
        {
            static_cast<T &>(*this).ThreadPool().InitStorages(Callback);
            return static_cast<T &>(*this);
        }

        // Router settings

        inline T &NoDelay(bool Enable)
        {
            Settings.NoDelay = Enable;
            return static_cast<T &>(*this);
        }

        inline T &Timeout(Duration const &timeout)
        {
            Settings.Timeout = timeout;
            return static_cast<T &>(*this);
        }

        inline T &MaxHeaderSize(size_t Size)
        {
            Settings.MaxHeaderSize = Size;
            return static_cast<T &>(*this);
        }

        inline T &MaxBodySize(size_t Size)
        {
            Settings.MaxBodySize = Size;
            return static_cast<T &>(*this);
        }

        inline T &RequestBufferSize(size_t Size)
        {
            Settings.RequestBufferSize = Size;
            return static_cast<T &>(*this);
        }

        inline T &ResponseBufferSize(size_t Size)
        {
            Settings.ResponseBufferSize = Size;
            return static_cast<T &>(*this);
        }

        inline T &RawContent(bool Value)
        {
            Settings.RawContent = Value;
            return static_cast<T &>(*this);
        }

        inline auto &Listen(Network::EndPoint const &endPoint)
        {
            return static_cast<T &>(*this).ListenWith(
                endPoint,
                [this, endPoint, Counter = 0ull](Async::EventLoop::Context &Context, ePoll::Entry &) mutable
                {
                    Network::Socket &Router = static_cast<Network::Socket &>(Context.Self.File);

                    auto [Client, Info] = Router.Accept();

                    if (!Client || !static_cast<T &>(*this).TryIncrementConnectionCount())
                        return;

                    // Set Non-blocking

                    Client.Blocking(false);

                    // Set NoDelay

                    if (Settings.NoDelay)
                        Client.SetOptions(IPPROTO_TCP, TCP_NODELAY, static_cast<int>(1));

                    static_cast<T &>(*this).ThreadPool()[Counter].Assign(
                        std::move(Client),
                        Async::EventLoop::CallbackType(std::type_identity<Connection>{}, Info, endPoint, Settings),
                        [this]
                        {
                            static_cast<T &>(*this).DecrementConnectionCount();
                        },
                        Settings.Timeout);

                    Counter = static_cast<T &>(*this).ThreadPool().Length() ? (Counter + 1) % static_cast<T &>(*this).ThreadPool().Length() : 0;
                },
                nullptr);
        }

        inline auto &Listen(Network::EndPoint const &endPoint, std::string_view Certification, std::string_view Key)
        {
            return static_cast<T &>(*this).ListenWith(
                endPoint,
                [this, endPoint, Counter = 0ull, TLS = TLSContext(Certification, Key)](Async::EventLoop::Context &Context, ePoll::Entry &) mutable
                {
                    Network::Socket &Router = static_cast<Network::Socket &>(Context.Self.File);

                    auto [Client, Info] = Router.Accept();

                    if (!Client || !static_cast<T &>(*this).TryIncrementConnectionCount())
                        return;

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

                    static_cast<T &>(*this).ThreadPool()[Counter].Assign(
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
                                Context.Upgrade(Async::EventLoop::CallbackType(std::type_identity<Connection>{}, Info, endPoint, Settings, std::move(SSL)), Settings.Timeout);
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
                            static_cast<T &>(*this).DecrementConnectionCount();
                        },
                        Settings.Timeout);

                    Counter = static_cast<T &>(*this).ThreadPool().Length() ? (Counter + 1) % static_cast<T &>(*this).ThreadPool().Length() : 0;
                },
                nullptr);
        }

        void Default(HTTP::Connection::Context &Context, HTTP::Request &Request)
        {
            _Router.Default(Context, Request);
        }

    private:
        Connection::Settings Settings{
            1024 * 1024 * 1,
            1024 * 1024 * 5,
            1024,
            1024,
            [this](Connection::Context &Context, Network::HTTP::Request &Request)
            {
                _Router.Match(Request.Path, Request.Method, Context, Request);
            },
            [](Connection::Context &Context, HTTP::Response &&Response, File &&file, size_t FileLength)
            {
                HTTP::Connection &Connection = Context.HandlerAs<HTTP::Connection>();

                size_t StringLength = 0;
                auto Buffer = Iterable::Queue<char>(Connection.Setting.ResponseBufferSize);
                Format::Stream Ser(Buffer);

                // Serialize first line

                Ser << "HTTP/" << Response.Version << ' ' << std::to_string(static_cast<unsigned short>(Response.Status)) << ' ' << Response.Brief << "\r\n";

                // Serialize headers

                for (auto const &[k, v] : Response.Headers)
                    Ser << k << ": " << v << "\r\n";

                // Handle keep-alive

                if (!Connection.ShouldClose && Response.Version == HTTP::HTTP10)
                {
                    Ser << "connection: keep-alive\r\n";
                }
                else if (Connection.ShouldClose && Response.Version == HTTP::HTTP11)
                {
                    Ser << "connection: close\r\n";
                }

                // Calculate length

                if (file)
                {
                    FileLength = FileLength ? FileLength : file.BytesLeft();
                }
                else
                {
                    StringLength = Response.Content.length();
                }

                if (Response.Headers.find("content-length") == Response.Headers.end())
                    Ser << "content-length: " << std::to_string(FileLength + StringLength) << "\r\n";

                Response.SetCookies.ForEach(
                    [&](auto const &Cookie)
                    {
                        Ser << "set-cookie: " << Cookie << "\r\n";
                    });

                Ser << "\r\n"
                    << Response.Content;

                Connection.AppendBuffer(std::move(Buffer), std::move(file), FileLength);
            },
            false,
            false,
            {5, 0}};

        ::Router<void(HTTP::Connection::Context &, HTTP::Request &)> _Router;

        static void DefaultRoute(HTTP::Connection::Context &Context, HTTP::Request &Request)
        {
            Context.SendResponse(HTTP::Response::HTML(Request.Version, HTTP::Status::NotFound, "<h1>404 Not Found</h1>"));
        }
    };
}