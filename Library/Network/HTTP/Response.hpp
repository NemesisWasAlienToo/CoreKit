#pragma once

#include <iostream>
#include <string>
#include <map>

#include <Duration.hpp>
#include <DateTime.hpp>
#include <Network/HTTP/HTTP.hpp>
#include <Iterable/List.hpp>
#include <Format/Stringifier.hpp>

namespace Core
{
    namespace Network
    {
        namespace HTTP
        {
            class Response : public Common
            {
            public:
                HTTP::Status Status;
                std::string Brief;
                Iterable::List<std::string> SetCookies;

                Iterable::Queue<char> ToBuffer()
                {
                    Iterable::Queue<char> Result(20);

                    Format::Stringifier Ser(Result);

                    Ser << "HTTP/" << Version << ' ' << std::to_string(static_cast<unsigned short>(Status)) << ' ' << Brief << "\r\n";

                    for (auto const &kv : Headers)
                        Ser << kv.first + ": " << kv.second << "\r\n";

                    SetCookies.ForEach([&Ser](std::string const &Cookie)
                    {
                        Ser << "Set-Cookie: " << Cookie << "\r\n";
                    });

                    Ser << "\r\n" << Content;

                    return Result;
                }

                std::string ToString() const
                {
                    std::stringstream ss;

                    ss << "HTTP/" << Version << " " << std::to_string(static_cast<unsigned short>(Status)) << " " << Brief << "\r\n";
                    
                    for (auto const &kv : Headers)
                        ss << kv.first + ": " << kv.second << "\r\n";

                    SetCookies.ForEach([&ss](std::string const &Cookie)
                    {
                        ss << "Set-Cookie: " << Cookie << "\r\n";
                    });

                    ss << "\r\n" << Content;

                    return ss.str();
                }

                Response& SetCookie(const std::string &Name, const std::string &Value,
                               const std::string &Path = "", const std::string &Domain = "",
                               bool Secure = false, bool HttpOnly = false)
                {
                    std::stringstream ResultStream;

                    ResultStream << Name << "=" << Value;

                    if (!Path.empty())
                    {
                        ResultStream << "; Path=" << Path;
                    }

                    if (!Domain.empty())
                    {
                        ResultStream << "; Domain=" << Domain;
                    }

                    if (Secure)
                    {
                        ResultStream << "; Secure";
                    }

                    if (HttpOnly)
                    {
                        ResultStream << "; HttpOnly";
                    }

                    SetCookies.Add(ResultStream.str());

                    return *this;
                }

                Response& SetCookie(const std::string &Name, const std::string &Value, size_t MaxAge,
                               const std::string &Path = "", const std::string &Domain = "",
                               bool Secure = false, bool HttpOnly = false)
                {
                    std::stringstream ResultStream;

                    ResultStream << Name << "=" << Value;

                    ResultStream << "; Max-Age=" << MaxAge;

                    if (!Path.empty())
                    {
                        ResultStream << "; Path=" << Path;
                    }

                    if (!Domain.empty())
                    {
                        ResultStream << "; Domain=" << Domain;
                    }

                    if (Secure)
                    {
                        ResultStream << "; Secure";
                    }

                    if (HttpOnly)
                    {
                        ResultStream << "; HttpOnly";
                    }

                    SetCookies.Add(ResultStream.str());

                    return *this;
                }

                Response& SetCookie(const std::string &Name, const std::string &Value, const DateTime &Expires,
                               const std::string &Path = "", const std::string &Domain = "",
                               bool Secure = false, bool HttpOnly = false)
                {
                    // Expires=Thu, 21 Oct 2021 07:28:00 GMT;

                    std::stringstream ResultStream;

                    ResultStream << Name << "=" << Value;

                    ResultStream << "; Expires=" << Expires.ToGMT().Format("%a, %d %b %Y %T %X GMT");

                    if (!Path.empty())
                    {
                        ResultStream << "; Path=" << Path;
                    }

                    if (!Domain.empty())
                    {
                        ResultStream << "; Domain=" << Domain;
                    }

                    if (Secure)
                    {
                        ResultStream << "; Secure";
                    }

                    if (HttpOnly)
                    {
                        ResultStream << "; HttpOnly";
                    }

                    SetCookies.Add(ResultStream.str());

                    return *this;
                }

                Response& RemoveCookie(const std::string &Name)
                {
                    std::stringstream ResultStream;

                    ResultStream << Name << "=";

                    ResultStream << "; Max-Age=-1";

                    SetCookies.Add(ResultStream.str());

                    return *this;
                }

                static Response From(const std::string &Text, size_t BodyIndex = 0)
                {
                    size_t Cursor = 5;
                    Response ret;

                    auto _Sepration = BodyIndex ? BodyIndex : Text.find("\r\n\r\n");

                    // Pars headers

                    {
                        size_t CursorTmp = 0;

                        // Pars first line

                        // Pars version

                        CursorTmp = Text.find(' ', Cursor);
                        ret.Version = Text.substr(Cursor, CursorTmp - Cursor);
                        Cursor = CursorTmp + 1;

                        // Pars code

                        CursorTmp = Text.find(' ', Cursor);
                        ret.Status = HTTP::Status(std::stoul(Text.substr(Cursor, CursorTmp - Cursor)));
                        Cursor = CursorTmp + 1;

                        // Pars breif

                        CursorTmp = Text.find("\r\n", Cursor);
                        ret.Brief = Text.substr(Cursor, CursorTmp - Cursor);
                        Cursor = CursorTmp + 2;
                    }

                    // Pars the rest of the headers

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

                // @todo Optimize rvalue

                static Response From(HTTP::Status Status, std::string Version = "1.0", std::map<std::string, std::string> Headers = {}, std::string Content = "")
                {
                    Response response;
                    response.Status = Status;
                    response.Version = std::move(Version);
                    response.Headers = std::move(Headers);
                    response.Content = std::move(Content);
                    response.Brief = StatusMessage.at(Status);
                    return response;
                }

                static Response Text(HTTP::Status Status, std::string Content)
                {
                    return From(Status, "1.0", {{"Content-Type", "text/plain"}}, std::move(Content));
                }

                static Response HTML(HTTP::Status Status, std::string Content)
                {
                    auto size = Content.size();
                    return From(Status, "1.0", {{"Content-Type", "text/html"}, {"Content-Length", std::to_string(size)}}, std::move(Content));
                }

                static Response Json(HTTP::Status Status, std::string Content)
                {
                    auto size = Content.size();
                    return From(Status, "1.0", {{"Content-Type", "application/json"}, {"Content-Length", std::to_string(size)}}, std::move(Content));
                }

                static Response Redirect(HTTP::Status Status, std::string Location)
                {
                    return From(Status, "1.0", {{"Location", std::move(Location)}}, "");
                }

                static Response Redirect(std::string Location)
                {
                    return Redirect(HTTP::Status::Found, std::move(Location));
                }
            };
        }
    }
}