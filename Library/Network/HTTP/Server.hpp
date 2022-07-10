#pragma once

#include <string>
#include <optional>

#include <Network/DNS.hpp>
#include <Event.hpp>
#include <Duration.hpp>
#include <ePoll.hpp>
#include <Iterable/List.hpp>
#include <Network/HTTP/HTTP.hpp>
#include <Format/Stream.hpp>
#include <Network/HTTP/Response.hpp>
#include <Network/HTTP/Request.hpp>
#include <Network/HTTP/Parser.hpp>
#include <Network/TCPServer.hpp>
#include <Network/HTTP/Router.hpp>
#include <Network/HTTP/ConnectionHandler.hpp>

namespace Core::Network::HTTP
{
    class Server
    {
    public:
        Server() = default;
        Server(EndPoint const &endPoint, Duration const &timeout, size_t ThreadCount = std::thread::hardware_concurrency(), Duration const &Interval = Duration::FromMilliseconds(500))
            : TCP(
                  endPoint,
                  [this](Network::EndPoint const &Target)
                  {
                      return ConnectionHandler(Timeout, Target, Settings);
                  },
                  timeout,
                  ThreadCount,
                  Interval),
              Timeout(timeout)
        {
        }

        // Server functions

        inline Async::ThreadPool &ThreadPool()
        {
            return TCP.ThreadPool();
        }

        inline Server &Run()
        {
            TCP.Run();
            return *this;
        }

        inline void GetInPool()
        {
            TCP.GetInPool();
        }

        inline void Stop()
        {
            TCP.Stop();
        }

        inline static void SendResponse(HTTP::ConnectionContext const &Context, HTTP::Response &&Response)
        {
            Context.Self.CallbackAs<HTTP::ConnectionHandler>()->AppendResponse(std::move(Response));
            Context.ListenFor(ePoll::Out | ePoll::In);
        }

        template <typename TAction>
        inline Server &SetDefault(TAction &&Action)
        {
            _Router.Default = std::forward<TAction>(Action);
            return *this;
        }

        template <ctll::fixed_string TRoute, bool Group = false, typename TAction>
        inline Server &SetBind(HTTP::Methods Method, TAction &&Action)
        {
            _Router.Add<TRoute, Group>(Method, std::forward<TAction>(Action));
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
            TCP.InitStorages(Callback);
            return *this;
        }

        template <typename TCallback>
        inline Server &OnError(TCallback &&Callback)
        {
            Settings.OnError = std::forward<TCallback>(Callback);
            return *this;
        }

        // Server settings

        inline Server &MaxConnectionCount(size_t Count)
        {
            TCP.MaxConnectionCount(Count);
            return *this;
        }

        inline Server &NoDelay(bool Enable)
        {
            TCP.NoDelay(Enable);
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

        inline Server &MaxFileSize(size_t Size)
        {
            Settings.MaxFileSize = Size;
            return *this;
        }

        // Zero means no sendfile will be used

        inline Server &SendFileThreshold(size_t Size)
        {
            Settings.SendFileThreshold = Size;
            return *this;
        }

        inline Server &HostName(std::string Name)
        {
            Settings.HostName = std::move(Name);
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

    private:
        TCPServer TCP;
        Router<std::optional<HTTP::Response>(HTTP::ConnectionContext &, Network::HTTP::Request &)> _Router;
        Duration Timeout;

    public:
        ConnectionHandler::Settings Settings{
            1024 * 1024 * 1,
            1024 * 1024 * 5,
            1024 * 1024 * 5,
            1024 * 10,
            1024,
            1024,
            Network::DNS::HostName(),
            nullptr,
            [this](ConnectionContext &Context, Network::HTTP::Request &Request)
            {
                return _Router.Match(Request, Context, Request);
            }};
    };
}