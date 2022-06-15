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

                void Run()
                {
                    TCP.Run();
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

                using TFilter = std::function<HTTP::Response(Network::EndPoint const &Target, Network::HTTP::Request &, std::shared_ptr<void>)>;

                template <typename TBuildCallback>
                inline void FilterFrom(TBuildCallback &&Builder)
                {
                    if (!Filters)
                    {
                        Filters = Builder(
                            [this](Network::EndPoint const &Target, Network::HTTP::Request &Request, std::shared_ptr<void> DataPtr)
                            {
                                return _Router.Match(Request.Method, Request.Path, Target, Request, DataPtr);
                            });

                        return;
                    }

                    Filters = Builder(std::move(Filters));
                }

            private:
                TCPServer TCP;
                Router<HTTP::Response(Network::EndPoint const &, Network::HTTP::Request const &, std::shared_ptr<void>)> _Router;
                Duration Timeout;

                TFilter Filters = nullptr;

            public:
                inline HTTP::Response OnRequest(Network::EndPoint const &Target, Network::HTTP::Request &Request) const
                {
                    if (!Filters)
                        return _Router.Match(Request.Method, Request.Path, Target, Request, nullptr);

                    return Filters(Target, Request, nullptr);
                }
            };
        }
    }
}