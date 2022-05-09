#pragma once

#include <iostream>
#include <string>
#include <map>
#include <regex>

#include <Duration.hpp>
#include <Iterable/Queue.hpp>
#include <Network/Socket.hpp>
#include <Format/Serializer.hpp>
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
                enum class Methods : unsigned short
                {
                    GET = 0,
                    POST,
                    PUT,
                    DELETE,
                    HEAD,
                    OPTIONS,
                    PATCH,
                    Any
                };

                std::string Type;
                std::string Path;

                Iterable::Queue<char> ToBuffer()
                {
                    Iterable::Queue<char> Result(20);

                    Format::Stringifier Ser(Result);

                    Ser << Type << ' ' << Path << " HTTP/" << Version << "\r\n";

                    for (auto const &[k, v] : Headers)
                        Ser << k << ": " << v << "\r\n";

                    Ser << "\r\n"
                        << Content;

                    return Result;
                }

                std::string ToString() const
                {
                    std::stringstream ss;

                    ss << Type << ' ' << Path << " HTTP/" << Version << "\r\n";

                    for (auto const &[k, v] : Headers)
                        ss << k << ": " << v << "\r\n";

                    ss << "\r\n"
                       << Content;

                    return ss.str();
                }

                void Cookies(std::map<std::string, std::string> &Result) const
                {
                    auto Iterator = Headers.find("Cookie");

                    if (Iterator == Headers.end())
                    {
                        return;
                    }

                    const std::string &Cookie = Iterator->second;

                    std::regex Capture("([^;]+)=([^;]+)(?:;\\s)?");

                    auto QueryBegin = std::sregex_iterator(Cookie.begin(), Cookie.end(), Capture);
                    auto QueryEnd = std::sregex_iterator();

                    std::smatch Matches;

                    for (std::sregex_iterator i = QueryBegin; i != QueryEnd; ++i)
                    {
                        Result[i->str(1)] = i->str(2);
                    }
                }

                std::map<std::string, std::string> Cookies() const
                {
                    std::map<std::string, std::string> Result;

                    Cookies(Result);

                    return Result;
                }

                void FormData(std::map<std::string, std::string> &Result) const
                {
                    std::regex Capture("([^&]+)=([^&]+)");

                    auto QueryBegin = std::sregex_iterator(Content.begin(), Content.end(), Capture);
                    auto QueryEnd = std::sregex_iterator();

                    std::smatch Matches;

                    for (std::sregex_iterator i = QueryBegin; i != QueryEnd; ++i)
                    {
                        std::cout << i->str(1) << " : " << i->str(2) << std::endl;
                        Result[i->str(1)] = i->str(2);
                    }
                }

                std::map<std::string, std::string> FormData() const
                {
                    std::map<std::string, std::string> Result;

                    FormData(Result);

                    return Result;
                }

                size_t ParseFirstLine(std::string_view const &Text) override
                {
                    // @todo Hanlde null case

                    size_t Cursor = 0;
                    size_t CursorTmp = 0;

                    // Parse method

                    if ((CursorTmp = Text.find(' ', Cursor)) == std::string::npos)
                    {
                        Type = std::string(Text.substr(Cursor));
                        return Text.length();
                    }

                    Type = Text.substr(Cursor, CursorTmp - Cursor);
                    Cursor = CursorTmp + 1;

                    // Parse path

                    if ((CursorTmp = Text.find(' ', Cursor)) == std::string::npos)
                    {
                        Path = Text.substr(Cursor);
                        return Text.length();
                    }

                    Path = Text.substr(Cursor, CursorTmp - Cursor);
                    Cursor = CursorTmp + 6;

                    // Parse version

                    if ((CursorTmp = Text.find('\r', Cursor)) == std::string::npos)
                    {
                        Version = Text.substr(Cursor);
                        return Text.length();
                    }

                    Version = Text.substr(Cursor, CursorTmp - Cursor);
                    // Cursor = CursorTmp + 2;

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

                inline static Request From(std::string Version, std::string Method, std::string Path, std::map<std::string, std::string> Headers = {}, std::string Content = "")
                {
                    Request request;
                    request.Type = std::move(Method);
                    request.Path = std::move(Path);
                    request.Version = std::move(Version);
                    request.Headers = std::move(Headers);
                    request.Content = std::move(Content);
                    return request;
                }

                static Request Get(std::string Version, const std::string &Path)
                {
                    return Request::From(
                        std::move(Version),
                        "GET",
                        Path);
                }

                static Request Post(std::string Version, std::string Path, std::string Content)
                {
                    return Request::From(
                        std::move(Version),
                        "POST",
                        Path,
                        {{"Content-Type", "application/x-www-form-urlencoded"},
                         {"Content-Length", std::to_string(Content.size())}},
                        Content);
                }

                static Request Put(std::string Version, const std::string &Path, const std::string &Content)
                {
                    return Request::From(
                        std::move(Version),
                        "PUT",
                        Path,
                        {{"Content-Type", "application/x-www-form-urlencoded"},
                         {"Content-Length", std::to_string(Content.size())}},
                        Content);
                }

                static Request Delete(std::string Version, const std::string &Path)
                {
                    return Request::From(
                        std::move(Version),
                        "DELETE",
                        Path);
                }

                static Request Options(std::string Version, const std::string &Path)
                {
                    return Request::From(
                        std::move(Version),
                        "OPTIONS",
                        Path);
                }

                static Request Head(std::string Version, const std::string &Path)
                {
                    return Request::From(
                        std::move(Version),
                        "HEAD",
                        Path);
                }

                static Request Patch(std::string Version, const std::string &Path, const std::string &Content)
                {
                    return Request::From(
                        std::move(Version),
                        "PATCH",
                        Path,
                        {{"Content-Length", std::to_string(Content.size())}},
                        Content);
                }
            };
        }
    }
}