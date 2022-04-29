#pragma once

#include <iostream>
#include <string>
#include <map>

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
            class Request : public Common
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
                    TRACE,
                    CONNECT,
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

                    for (auto const &kv : Headers)
                        Ser << kv.first << ": " << kv.second << "\r\n";

                    Ser << "\r\n"
                        << Content;

                    return Result;
                }

                std::string ToString() const
                {
                    std::stringstream ss;

                    ss << Type << ' ' << Path << " HTTP/" << Version << "\r\n";

                    for (auto const &kv : Headers)
                        ss << kv.first << ": " << kv.second << "\r\n";

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

                void applicationForm(std::map<std::string, std::string> &Result) const
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

                void multipartForm(const std::string &Boundry, std::map<std::string, std::string> &Result) const
                {
                    std::regex Capture(Boundry + "\r\nContent-Disposition:\\s?form-data;\\s?name=\"([^\"]+)\"\r\n\r\n([^-\n]+)");

                    auto QueryBegin = std::sregex_iterator(Content.begin(), Content.end(), Capture);
                    auto QueryEnd = std::sregex_iterator();

                    std::smatch Matches;

                    for (std::sregex_iterator i = QueryBegin; i != QueryEnd; ++i)
                    {
                        Result[i->str(1)] = i->str(2);
                    }
                }

                void multipartForm(std::map<std::string, std::string> &Result) const
                {
                    std::regex Capture("name=\"([^\"]+)\"\r\n\r\n([^-\n]+)");

                    auto QueryBegin = std::sregex_iterator(Content.begin(), Content.end(), Capture);
                    auto QueryEnd = std::sregex_iterator();

                    std::smatch Matches;

                    for (std::sregex_iterator i = QueryBegin; i != QueryEnd; ++i)
                    {
                        Result[i->str(1)] = i->str(2);
                    }
                }

                void FormData(std::map<std::string, std::string> &Result) const
                {
                    std::smatch Matches;

                    auto Iterator = Headers.find("Content-Type");

                    if (Iterator == Headers.end())
                    {
                        return;
                    }

                    // @todo Optimize this later

                    if (std::regex_match(Iterator->second, Matches, std::regex("application/x-www-form-urlencoded")))
                    {
                        applicationForm(Result);
                    }
                    else if (std::regex_match(Iterator->second, Matches, std::regex("multipart/form-data; boundary=\"?([^\"]+)\"?")))
                    {
                        auto BoundryString = Matches.str(1);
                        // multipartForm(BoundryString, Content, Result);
                        multipartForm(Content, Result);
                    }
                }

                Response Send(const EndPoint Target, Duration TimeOut = {0, 0})
                {
                    Network::Socket client(Network::Socket::IPv4, Network::Socket::TCP);

                    client.Connect(Target);

                    client.Await(Network::Socket::Out);

                    Iterable::Queue<char> buffer;
                    Format::Serializer Ser(buffer);

                    Ser << ToString();

                    while (!buffer.IsEmpty())
                    {
                        client << Ser;
                    }

                    Ser.Clear();

                    client.Await(Network::Socket::Out);

                    for (client.Await(Network::Socket::In); client.Received() > 0; client.Await(Network::Socket::In))
                    {
                        client >> Ser;
                    }

                    client.Close();

                    return Response::From(Ser.Take<std::string>());
                }

                static Request From(const std::string &Text, size_t BodyIndex = 0)
                {
                    size_t Cursor = 0;
                    Request ret;

                    auto _Sepration = BodyIndex ? BodyIndex : Text.find("\r\n\r\n");

                    // Pars headers

                    {
                        // Pars first line

                        // Pars method

                        size_t CursorTmp = Text.find(' ', Cursor);
                        ret.Type = Text.substr(Cursor, CursorTmp - Cursor);
                        Cursor = CursorTmp + 1;

                        // Pars path

                        CursorTmp = Text.find(" HTTP/", Cursor);
                        ret.Path = Text.substr(Cursor, CursorTmp - Cursor);
                        Cursor = CursorTmp + 6;

                        // Pars version

                        CursorTmp = Text.find("\r\n", Cursor);
                        ret.Version = Text.substr(Cursor, CursorTmp - Cursor);
                        Cursor = CursorTmp + 2;
                    }

                    // Pars the rest of the headers

                    // @todo Handle multiple cookie

                    {
                        while (Cursor < _Sepration)
                        {
                            // Find key

                            size_t CursorTmp = Text.find(": ", Cursor);
                            std::string HeaderKey = Text.substr(Cursor, CursorTmp - Cursor);
                            Cursor = CursorTmp + 2;

                            // Find value

                            CursorTmp = Text.find("\r\n", Cursor);
                            std::string HeaderValue = Text.substr(Cursor, CursorTmp - Cursor);
                            Cursor = CursorTmp + 2;

                            ret.Headers.insert_or_assign(std::move(HeaderKey), std::move(HeaderValue));
                        }
                    }

                    // Pars the body

                    ret.Content = Text.substr(_Sepration + 4);

                    return ret;
                }

                inline static Request From(std::string Method, std::string Path, std::string Version = "1.0", std::map<std::string, std::string> Headers = {}, std::string Content = "")
                {
                    Request request;
                    request.Type = std::move(Method);
                    request.Path = std::move(Path);
                    request.Version = std::move(Version);
                    request.Headers = std::move(Headers);
                    request.Content = std::move(Content);
                    return request;
                }

                inline static Request From(Methods Method, std::string Path, std::string Version = "1.0", std::map<std::string, std::string> Headers = {}, std::string Content = "")
                {
                    return From(MethodStrings[static_cast<unsigned short>(Method)], std::move(Path), std::move(Version), std::move(Headers), std::move(Content));
                }

                static Request From(const std::string &Method, const std::string &Path, const std::string &Content)
                {
                    return Request::From(
                        Method,
                        Path,
                        "1.0",
                        {{"Content-Length", std::to_string(Content.size())}},
                        Content);
                }

                static Request Get(const std::string &Path)
                {
                    return Request::From(
                        "GET",
                        Path);
                }

                static Request Post(const std::string &Path, const std::string &Content)
                {
                    return Request::From(
                        "POST",
                        Path,
                        "1.0",
                        {{"Content-Length", std::to_string(Content.size())}},
                        Content);
                }

                static Request Put(const std::string &Path, const std::string &Content)
                {
                    return Request::From(
                        "PUT",
                        Path,
                        "1.0",
                        {{"Content-Length", std::to_string(Content.size())}},
                        Content);
                }

                static Request Delete(const std::string &Path)
                {
                    return Request::From(
                        "DELETE",
                        Path,
                        "1.0",
                        {{"Host", Network::DNS::HostName()}});
                }

                static Request Options(const std::string &Path)
                {
                    return Request::From(
                        "OPTIONS",
                        Path,
                        "1.0",
                        {{"Host", Network::DNS::HostName()}});
                }

                static Request Head(const std::string &Path)
                {
                    return Request::From(
                        "HEAD",
                        Path,
                        "1.0",
                        {{"Host", Network::DNS::HostName()}});
                }

                static Request Trace(const std::string &Path)
                {
                    return Request::From(
                        "TRACE",
                        Path,
                        "1.0",
                        {{"Host", Network::DNS::HostName()}});
                }

                static Request Connect(const std::string &Path)
                {
                    return Request::From(
                        "CONNECT",
                        Path,
                        "1.0",
                        {{"Host", Network::DNS::HostName()}});
                }

                static Request Patch(const std::string &Path, const std::string &Content)
                {
                    return Request::From(
                        "PATCH",
                        Path,
                        "1.0",
                        {{"Content-Length", std::to_string(Content.size())}},
                        Content);
                }
            };
        }
    }
}