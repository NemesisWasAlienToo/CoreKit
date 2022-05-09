#pragma once

#include <iostream>
#include <string>
#include <regex>
#include <map>

#include <Network/HTTP/HTTP.hpp>
#include <Network/HTTP/Response.hpp>
#include <Network/HTTP/Request.hpp>
#include <Iterable/List.hpp>
#include <Iterable/Span.hpp>

namespace Core
{
    namespace Network
    {
        namespace HTTP
        {
            class Router
            {
            public:
                using Handler = std::function<HTTP::Response(const Network::EndPoint &, const Network::HTTP::Request &, std::map<std::string, std::string> &)>;
                // using Validator = std::function<bool(Network::EndPoint &, HTTP::Request &)>;

                struct Route
                {
                    Request::Methods Method;
                    std::regex Pattern;
                    Handler Action;
                    // Validator Filter;

                    Iterable::Span<std::string> Parameters;

                    // get query parameters

                    void Query(std::string::const_iterator Start, const std::string &Path, std::map<std::string, std::string> &Result) const
                    {
                        if (Start == Path.end())
                            return;

                        std::regex Capture("([^&]+)=([^&]+)");

                        auto QueryBegin = std::sregex_iterator(Start + 1, Path.end(), Capture);
                        auto QueryEnd = std::sregex_iterator();

                        std::smatch Matches;

                        for (std::sregex_iterator i = QueryBegin; i != QueryEnd; ++i)
                        {
                            Result[i->str(1)] = i->str(2);
                        }
                    }

                    static Route From(Request::Methods Method, const std::string& Pattern, Handler Action /*, Validator Filter = nullptr*/)
                    {
                        std::regex Parameter("\\[([^[/?]+)\\]");
                        std::regex Slash("/");

                        // Build matching capture

                        // @todo Optimize query parameter processing

                        std::string Capture = std::regex_replace(std::regex_replace(Pattern + "(?:\\?.*)?$", Parameter, "([^[/?]+)"), Slash, "\\/");

                        // Get the parameters

                        auto Begin = std::sregex_iterator(Pattern.begin(), Pattern.end(), Parameter);
                        auto End = std::sregex_iterator();

                        // @todo Optimize this

                        Iterable::Span<std::string> Parameters(std::distance(Begin, End));

                        int j = 0;

                        for (std::sregex_iterator i = Begin; i != End; ++i, ++j)
                        {
                            Parameters[j] = i->str(1);
                        }

                        // Create the capture regex

                        return {Method,
                                std::regex(std::move(Capture), std::regex_constants::icase),
                                std::move(Action),
                                std::move(Parameters)};
                    }

                    // @todo Optimize this

                    std::tuple<bool, std::map<std::string, std::string>> Match(const std::string &Path) const
                    {
                        std::map<std::string, std::string> Res;

                        std::smatch Matches;

                        // string iterator for holding the start posision of query parameters

                        std::string::const_iterator QueryParamsStartIndex = Path.begin();

                        if (!std::regex_match(Path, Matches, Pattern))
                        {
                            return {false, Res};
                        }

                        // Route parameters

                        for (size_t i = 0; i < Parameters.Length(); i++)
                        {
                            auto &Match = Matches[i + 1];

                            Res[Parameters[i]] = Match;

                            // Get tge position of start of query parameters

                            // @todo Optimize this

                            if (i == Parameters.Length() - 1)
                            {
                                // Advance the iterator to the start of query parameters

                                QueryParamsStartIndex = Match.second;
                            }
                        }

                        // QueryParamsStartIndex = Matches.back().second;

                        // Query parameters

                        Query(QueryParamsStartIndex, Path, Res);

                        return {true, Res};
                    }
                };

            private:
                Route DefaultRoute;
                Iterable::List<Route> Routes;

            public:
                Router() = default;

                inline void Default(Route route)
                {
                    DefaultRoute = std::move(route);
                }

                inline void Add(Route route)
                {
                    Routes.Add(std::move(route));
                }

                std::tuple<Route &, std::map<std::string, std::string>>
                Match(const std::string &Method, const std::string &Path)
                {
                    for (size_t i = 0; i < Routes.Length(); i++)
                    {
                        auto &Route = Routes[i];

                        if (Route.Method != HTTP::Request::Methods::Any && HTTP::MethodStrings[static_cast<unsigned short>(Routes[i].Method)] != Method)
                        {
                            continue;
                        }

                        auto [IsMatch, Parameters] = Route.Match(Path);

                        if (IsMatch)
                        {
                            return {Route, Parameters};
                        }
                    }

                    // @todo Optimize DefaultRoute Match to if the regex is empty its not tested

                    return {DefaultRoute, std::get<1>(DefaultRoute.Match(Path))};
                }

                const auto &Content()
                {
                    return Routes;
                }

                const auto &Default()
                {
                    return DefaultRoute;
                }
            };
        }
    }
}