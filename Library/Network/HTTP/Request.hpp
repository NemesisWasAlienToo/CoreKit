#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
// #include <Extra/compile-time-regular-expressions/single-header/ctre.hpp>

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

                friend Format::Stream &operator<<(Format::Stream &Ser, Request const &R)
                {
                    Ser << MethodStrings[size_t(R.Method)] << ' ' << R.Path << " HTTP/" << R.Version << "\r\n";

                    for (auto const &[k, v] : R.Headers)
                        Ser << k << ": " << v << "\r\n";

                    Ser << "\r\n"
                        << R.Content;

                    return Ser;
                }

                void SetContent(std::string_view NewContent)
                {
                    Content = std::string{NewContent};
                    Headers.insert_or_assign("Content-Length", std::to_string(Content.length()));
                }

                std::string ToString() const
                {
                    Iterable::Queue<char> Buffer;
                    Format::Stream Ser(Buffer);

                    Ser << *this;

                    return std::string{&Buffer.Head(), Buffer.Length()};
                }

                // void Cookies(std::unordered_map<std::string, std::string> &Result) const
                // {
                //     auto Iterator = Headers.find("Cookie");

                //     if (Iterator == Headers.end())
                //     {
                //         return;
                //     }

                //     std::string_view Cookie = Iterator->second;

                //     // for (auto match : ctre::range<"([^;]+)=([^;]+)(?:;\\s)?">(Cookie))
                //     for (auto match : ctre::range<"([^;]+)=([^;]+)">(Cookie))
                //     {
                //         Result.emplace(match.get<1>(), match.get<2>());
                //     }
                // }

                // std::unordered_map<std::string, std::string> Cookies() const
                // {
                //     std::unordered_map<std::string, std::string> Result;

                //     Cookies(Result);

                //     return Result;
                // }

                // void QueryParams(std::unordered_map<std::string, std::string> &Result) const
                // {
                //     std::string_view PathView{Path};

                //     auto Start = PathView.find('?');

                //     if (Start == std::string_view::npos || PathView.length() == ++Start)
                //     {
                //         return;
                //     }

                //     std::string_view Params = PathView.substr(Start);

                //     for (auto match : ctre::range<"([^&]+)=([^&]+)">(Params))
                //     {
                //         Result.emplace(match.get<1>(), match.get<2>());
                //     }
                // }

                // void QueryParamViews(std::unordered_map<std::string, std::string_view> &Result) const
                // {
                //     std::string_view PathView{Path};

                //     auto Start = PathView.find('?');

                //     if (Start == std::string_view::npos || PathView.length() == ++Start)
                //     {
                //         return;
                //     }

                //     std::string_view Params = PathView.substr(Start);

                //     for (auto match : ctre::range<"([^&]+)=([^&]+)">(Params))
                //     {
                //         Result.emplace(match.get<1>(), match.get<2>());
                //     }
                // }

                // std::unordered_map<std::string, std::string_view> QueryParamViews() const
                // {
                //     std::unordered_map<std::string, std::string_view> Result;

                //     QueryParamViews(Result);

                //     return Result;
                // }

                // std::unordered_map<std::string, std::string> QueryParams() const
                // {
                //     std::unordered_map<std::string, std::string> Result;

                //     QueryParams(Result);

                //     return Result;
                // }

                // void FormData(std::unordered_map<std::string, std::string> &Result) const
                // {
                //     for (auto match : ctre::range<"([^&]+)=([^&]+)">(Content))
                //     {
                //         // @todo Optimize string

                //         Result.emplace(match.get<1>(), match.get<2>());
                //     }
                // }

                // std::unordered_map<std::string, std::string> FormData() const
                // {
                //     std::unordered_map<std::string, std::string> Result;

                //     FormData(Result);

                //     return Result;
                // }

                size_t ParseFirstLine(std::string_view Text)
                {
                    // @todo Handle null case

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

                static Request From(std::string_view Text, size_t BodyIndex = 0)
                {
                    Request ret;
                    size_t Index = 0;

                    Index = ret.ParseFirstLine(Text);
                    Index = ret.ParseHeaders(Text, Index, BodyIndex - 4);
                    ret.ParseContent(Text, Index);

                    return ret;
                }

                inline static Request From(std::string_view Version, Methods Method, std::string_view Path, std::unordered_map<std::string, std::string> Headers = {}, std::string_view Content = "")
                {
                    Request request;
                    request.Method = Method;
                    request.Path = Path;
                    request.Version = Version;
                    request.Headers = std::move(Headers);
                    request.Content = Content;
                    return request;
                }

                static Request Get(std::string_view Version, std::string_view Path)
                {
                    return Request::From(
                        Version,
                        Methods::GET,
                        Path);
                }

                static Request Post(std::string_view Version, std::string_view Path, std::string_view Content)
                {
                    return Request::From(
                        Version,
                        Methods::POST,
                        Path,
                        {{"Content-Type", "application/x-www-form-urlencoded"},
                         {"Content-Length", std::to_string(Content.size())}},
                        Content);
                }

                static Request Put(std::string_view Version, std::string_view Path, std::string_view Content)
                {
                    return Request::From(
                        Version,
                        Methods::PUT,
                        Path,
                        {{"Content-Type", "application/x-www-form-urlencoded"},
                         {"Content-Length", std::to_string(Content.size())}},
                        Content);
                }

                static Request Delete(std::string_view Version, std::string_view Path)
                {
                    return Request::From(
                        Version,
                        Methods::DELETE,
                        Path);
                }

                static Request Options(std::string_view Version, std::string_view Path)
                {
                    return Request::From(
                        Version,
                        Methods::OPTIONS,
                        Path);
                }

                static Request Head(std::string_view Version, std::string_view Path)
                {
                    return Request::From(
                        Version,
                        Methods::HEAD,
                        Path);
                }

                static Request Patch(std::string_view Version, std::string_view Path, std::string_view Content)
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