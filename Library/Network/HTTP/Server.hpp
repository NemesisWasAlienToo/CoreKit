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
                    size_t MaxConnectionCount;
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

                // Server& MaxHeaderSize(size_t Size)
                // {
                //     Settings.MaxHeaderSize = Size;
                //     return *this;
                // }

                // Server& MaxBodySize(size_t Size)
                // {
                //     Settings.MaxBodySize = Size;
                //     return *this;
                // }

                // Server& MaxConnectionCount(size_t Count)
                // {
                //     Settings.MaxConnectionCount = Count;
                //     return *this;
                // }

                // Server& MaxFileSize(size_t Size)
                // {
                //     Settings.MaxFileSize = Size;
                //     return *this;
                // }

                // Server& SendFileThreshold(size_t Size)
                // {
                //     Settings.SendFileThreshold = Size;
                //     return *this;
                // }

                // Server functions

                Server& Run()
                {
                    TCP.Run();
                    return *this;
                }

                void GetInPool()
                {
                    TCP.GetInPool();
                }

                void Stop()
                {
                    TCP.Stop();
                }

                template <typename TAction>
                inline void SetDefault(TAction &&Action)
                {
                    _Router.Default = std::forward<TAction>(Action);
                }

                template <ctll::fixed_string TRoute, bool Group = false, typename TAction>
                inline void Set(HTTP::Methods Method, TAction &&Action)
                {
                    _Router.Add<TRoute, Group>(Method, std::forward<TAction>(Action));
                }

                template <ctll::fixed_string TRoute, bool Group = false, typename TAction>
                inline void GET(TAction &&Action)
                {
                    Set<TRoute, Group>(
                        HTTP::Methods::GET,
                        std::forward<TAction>(Action));
                }

                template <ctll::fixed_string TRoute, bool Group = false, typename TAction>
                inline void POST(TAction &&Action)
                {
                    Set<TRoute, Group>(
                        HTTP::Methods::POST,
                        std::forward<TAction>(Action));
                }

                template <ctll::fixed_string TRoute, bool Group = false, typename TAction>
                inline void PUT(TAction &&Action)
                {
                    Set<TRoute, Group>(
                        HTTP::Methods::PUT,
                        std::forward<TAction>(Action));
                }

                template <ctll::fixed_string TRoute, bool Group = false, typename TAction>
                inline void DELETE(TAction &&Action)
                {
                    Set<TRoute, Group>(
                        HTTP::Methods::DELETE,
                        std::forward<TAction>(Action));
                }

                template <ctll::fixed_string TRoute, bool Group = false, typename TAction>
                inline void HEAD(TAction &&Action)
                {
                    Set<TRoute, Group>(
                        HTTP::Methods::HEAD,
                        std::forward<TAction>(Action));
                }

                template <ctll::fixed_string TRoute, bool Group = false, typename TAction>
                inline void OPTIONS(TAction &&Action)
                {
                    Set<TRoute, Group>(
                        HTTP::Methods::OPTIONS,
                        std::forward<TAction>(Action));
                }

                template <ctll::fixed_string TRoute, bool Group = false, typename TAction>
                inline void PATCH(TAction &&Action)
                {
                    Set<TRoute, Group>(
                        HTTP::Methods::PATCH,
                        std::forward<TAction>(Action));
                }

                template <ctll::fixed_string TRoute, bool Group = false, typename TAction>
                inline void Any(TAction &&Action)
                {
                    Set<TRoute, Group>(
                        HTTP::Methods::Any,
                        std::forward<TAction>(Action));
                }

                using TFilter = std::function<HTTP::Response(Network::EndPoint const &Target, Network::HTTP::Request &, std::shared_ptr<void> &)>;

                template <typename TBuildCallback>
                inline void FilterFrom(TBuildCallback &&Builder)
                {
                    if (!Filters)
                    {
                        Filters = Builder(
                            [this](Network::EndPoint const &Target, Network::HTTP::Request &Request, std::shared_ptr<void> &Storage)
                            {
                                return _Router.Match(Request.Method, Request.Path, Target, Request, Storage);
                            });

                        return;
                    }

                    Filters = Builder(std::move(Filters));
                }

                template <typename TCallback>
                inline Server& InitStorages(TCallback &&Callback)
                {
                    TCP.InitStorages(Callback);
                    return *this;
                }

            private:
                TCPServer TCP;
                Router<HTTP::Response(Network::EndPoint const &, Network::HTTP::Request const &, std::shared_ptr<void> &)> _Router;
                Duration Timeout;

                TFilter Filters = nullptr;
                Setting Settings;

            public:
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