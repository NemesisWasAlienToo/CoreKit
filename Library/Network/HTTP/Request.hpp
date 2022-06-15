#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <regex>
#include <ThirdParty/ctre.hpp>

#include <Duration.hpp>
#include <Iterable/Queue.hpp>
#include <Network/Socket.hpp>
#include <Format/Stream.hpp>
#include <Network/HTTP/Response.hpp>

namespace Core
{
    namespace Network
    {
        namespace HTTP
        {
            class Request : public Message
            {
            public:
                // enum class Methods : unsigned short
                // {
                //     GET = 0,
                //     POST,
                //     PUT,
                //     DELETE,
                //     HEAD,
                //     OPTIONS,
                //     PATCH,
                //     Any
                // };
                // static std::string const MethodStrings[];

                Methods Method;
                std::string Path;

                void AppendToBuffer(Iterable::Queue<char> &Result)
                {
                    Format::Stream Ser(Result);

                    Ser << MethodStrings[size_t(Method)] << ' ' << Path << " HTTP/" << Version << "\r\n";

                    for (auto const &[k, v] : Headers)
                        Ser << k << ": " << v << "\r\n";

                    Ser << "\r\n"
                        << Content;
                }

                Iterable::Queue<char> ToBuffer()
                {
                    Iterable::Queue<char> Result(20);

                    Format::Stream Ser(Result);

                    Ser << MethodStrings[size_t(Method)] << ' ' << Path << " HTTP/" << Version << "\r\n";

                    for (auto const &[k, v] : Headers)
                        Ser << k << ": " << v << "\r\n";

                    Ser << "\r\n"
                        << Content;

                    return Result;
                }

                std::string ToString() const
                {
                    std::stringstream ss;

                    ss << MethodStrings[size_t(Method)] << ' ' << Path << " HTTP/" << Version << "\r\n";

                    for (auto const &[k, v] : Headers)
                        ss << k << ": " << v << "\r\n";

                    ss << "\r\n"
                       << Content;

                    return ss.str();
                }

                void Cookies(std::unordered_map<std::string, std::string> &Result) const
                {
                    auto Iterator = Headers.find("Cookie");

                    if (Iterator == Headers.end())
                    {
                        return;
                    }

                    std::string_view Cookie = Iterator->second;

                    for (auto match : ctre::range<"([^;]+)=([^;]+)(?:;\\s)?">(Cookie))
                    {
                        Result.emplace(match.get<1>(), match.get<2>());
                    }
                }

                std::unordered_map<std::string, std::string> Cookies() const
                {
                    std::unordered_map<std::string, std::string> Result;

                    Cookies(Result);

                    return Result;
                }

                void QueryParams(std::unordered_map<std::string, std::string> &Result) const
                {
                    std::string_view PathView{Path};

                    auto Start = PathView.find('?');

                    if (Start == std::string_view::npos || PathView.length() == ++Start)
                    {
                        return;
                    }

                    std::string_view Params = PathView.substr(Start);

                    for (auto match : ctre::range<"([^&]+)=([^&]+)">(Params))
                    {
                        Result.emplace(match.get<1>(), match.get<2>());
                    }
                }

                std::unordered_map<std::string, std::string> QueryParams() const
                {
                    std::unordered_map<std::string, std::string> Result;

                    QueryParams(Result);

                    return Result;
                }

                void FormData(std::unordered_map<std::string, std::string> &Result) const
                {
                    for (auto match : ctre::range<"([^&]+)=([^&]+)">(Content))
                    {
                        // @todo Optimize string

                        Result.emplace(match.get<1>(), match.get<2>());
                    }
                }

                std::unordered_map<std::string, std::string> FormData() const
                {
                    std::unordered_map<std::string, std::string> Result;

                    FormData(Result);

                    return Result;
                }

                size_t ParseFirstLine(std::string_view const &Text)
                {
                    // @todo Hanlde null case

                    size_t Cursor = 0;
                    size_t CursorTmp = 0;

                    // Parse method

                    if ((CursorTmp = Text.find(' ', Cursor)) == std::string::npos)
                    {
                        throw std::invalid_argument("Invalid method");
                    }

                    Method = FromString(Text.substr(Cursor, CursorTmp - Cursor));

                    // Type = Text.substr(Cursor, CursorTmp - Cursor);
                    Cursor = CursorTmp + 1;

                    // Parse path

                    if ((CursorTmp = Text.find(' ', Cursor)) == std::string::npos)
                    {
                        throw std::invalid_argument("Invalid path");
                    }

                    Path = Text.substr(Cursor, CursorTmp - Cursor);
                    Cursor = CursorTmp + 6;

                    // Parse version

                    // @todo Maybe check the version's length?

                    if ((CursorTmp = Text.find('\r', Cursor)) == std::string::npos)
                    {
                        throw std::invalid_argument("Invalid version");
                    }

                    Version = Text.substr(Cursor, CursorTmp - Cursor);

                    return CursorTmp + 2;
                }

                static Request From(const std::string_view &Text, size_t BodyIndex = 0)
                {
                    Request ret;
                    size_t Index = 0;

                    Index = ret.ParseFirstLine(Text);
                    Index = ret.ParseHeaders(Text, Index, BodyIndex);
                    ret.ParseContent(Text, Index);

                    return ret;
                }

                inline static Request From(std::string_view const &Version, Methods Method, std::string_view const &Path, std::unordered_map<std::string, std::string> Headers = {}, std::string_view const &Content = "")
                {
                    Request request;
                    request.Method = Method;
                    request.Path = Path;
                    request.Version = Version;
                    request.Headers = std::move(Headers);
                    request.Content = Content;
                    return request;
                }

                static Request Get(std::string_view const &Version, std::string_view const &Path)
                {
                    return Request::From(
                        Version,
                        Methods::GET,
                        Path);
                }

                static Request Post(std::string_view const &Version, std::string_view const &Path, std::string_view const &Content)
                {
                    return Request::From(
                        Version,
                        Methods::POST,
                        Path,
                        {{"Content-Type", "application/x-www-form-urlencoded"},
                         {"Content-Length", std::to_string(Content.size())}},
                        Content);
                }

                static Request Put(std::string_view const &Version, std::string_view const &Path, std::string_view const &Content)
                {
                    return Request::From(
                        Version,
                        Methods::PUT,
                        Path,
                        {{"Content-Type", "application/x-www-form-urlencoded"},
                         {"Content-Length", std::to_string(Content.size())}},
                        Content);
                }

                static Request Delete(std::string_view const & Version, std::string_view const &Path)
                {
                    return Request::From(
                        Version,
                        Methods::DELETE,
                        Path);
                }

                static Request Options(std::string_view const &Version, std::string_view const &Path)
                {
                    return Request::From(
                        Version,
                        Methods::OPTIONS,
                        Path);
                }

                static Request Head(std::string_view const & Version, std::string_view const &Path)
                {
                    return Request::From(
                        Version,
                        Methods::HEAD,
                        Path);
                }

                static Request Patch(std::string_view const &Version, std::string_view const &Path, std::string_view const &Content)
                {
                    return Request::From(
                        Version,
                        Methods::PATCH,
                        Path,
                        {{"Content-Length", std::to_string(Content.size())}},
                        Content);
                }
            };

            // inline std::string const Request::MethodStrings[]{"GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "PATCH", ""};
        }
    }
}