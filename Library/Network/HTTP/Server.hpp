#pragma once

#include <string>

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

namespace Core
{
    namespace Network
    {
        namespace HTTP
        {
            class Server
            {
            public:
                struct Setting
                {
                    size_t MaxHeaderSize;
                    size_t MaxBodySize;
                    size_t MaxFileSize;
                    size_t SendFileThreshold;
                };

                Server() = default;
                Server(EndPoint const &endPoint, Duration const &timeout, size_t ThreadCount = std::thread::hardware_concurrency(), Duration const &Interval = Duration::FromMilliseconds(500))
                    : TCP(
                          endPoint,
                          [this](Network::EndPoint const &Target)
                          {
                              return ConnectionHandler(Timeout, Target, *this);
                          },
                          timeout,
                          ThreadCount,
                          Interval),
                      Timeout(timeout)
                {
                }

                // Server settings

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

                inline Server &MaxConnectionCount(size_t Count)
                {
                    TCP.MaxConnectionCount(Count);
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

                // Server functions

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

                using TFilter = std::function<HTTP::Response(Network::EndPoint const &Target, Network::HTTP::Request &, std::shared_ptr<void> &)>;

                template <typename TBuildCallback>
                inline Server &FilterFrom(TBuildCallback &&Builder)
                {
                    if (!Filters)
                    {
                        Filters = Builder(
                            [this](Network::EndPoint const &Target, Network::HTTP::Request &Request, std::shared_ptr<void> &Storage)
                            {
                                return _Router.Match(Request.Method, Request.Path, Target, Request, Storage);
                            });

                        return *this;
                    }

                    Filters = Builder(std::move(Filters));

                    return *this;
                }

                template <typename TCallback>
                inline Server &InitStorages(TCallback &&Callback)
                {
                    TCP.InitStorages(Callback);
                    return *this;
                }

            private:
                TCPServer TCP;
                Router<HTTP::Response(Network::EndPoint const &, Network::HTTP::Request const &, std::shared_ptr<void> &)> _Router;
                Duration Timeout;

                TFilter Filters = nullptr;

            public:
                Setting Settings{
                    .MaxHeaderSize = 1024 * 1024 * 1,
                    .MaxBodySize = 1024 * 1024 * 5,
                    .MaxFileSize = 1024 * 1024 * 5,
                    .SendFileThreshold = 1024 * 10};

                inline HTTP::Response OnRequest(Network::EndPoint const &Target, Network::HTTP::Request &Request, std::shared_ptr<void> &Storage) const
                {
                    if (!Filters)
                        return _Router.Match(Request.Method, Request.Path, Target, Request, Storage);

                    return Filters(Target, Request, Storage);
                }
            };
        }
    }
}