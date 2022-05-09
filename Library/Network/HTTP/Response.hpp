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
            class Response : public Message
            {
            public:
                HTTP::Status Status;
                std::string Brief;
                Iterable::List<std::string> SetCookies;

                Iterable::Queue<char> ToBuffer() const
                {
                    Iterable::Queue<char> Result(20);

                    Format::Stringifier Ser(Result);

                    Ser << "HTTP/" << Version << ' ' << std::to_string(static_cast<unsigned short>(Status)) << ' ' << Brief << "\r\n";

                    for (auto const &[k, v] : Headers)
                        Ser << k + ": " << v << "\r\n";

                    SetCookies.ForEach([&Ser](std::string const &Cookie)
                                       { Ser << "Set-Cookie: " << Cookie << "\r\n"; });

                    Ser << "\r\n"
                        << Content;

                    return Result;
                }

                std::string ToString() const
                {
                    std::stringstream ss;

                    ss << "HTTP/" << Version << " " << std::to_string(static_cast<unsigned short>(Status)) << " " << Brief << "\r\n";

                    for (auto const &[k, v] : Headers)
                        ss << k + ": " << v << "\r\n";

                    SetCookies.ForEach([&ss](std::string const &Cookie)
                                       { ss << "Set-Cookie: " << Cookie << "\r\n"; });

                    ss << "\r\n"
                       << Content;

                    return ss.str();
                }

                Response &SetCookie(const std::string &Name, const std::string &Value,
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

                Response &SetCookie(const std::string &Name, const std::string &Value, size_t MaxAge,
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

                Response &SetCookie(const std::string &Name, const std::string &Value, const Duration &MaxAge,
                                    const std::string &Path = "", const std::string &Domain = "",
                                    bool Secure = false, bool HttpOnly = false)
                {
                    std::stringstream ResultStream;

                    ResultStream << Name << "=" << Value;

                    ResultStream << "; Max-Age=" << MaxAge.Seconds;

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

                Response &SetCookie(const std::string &Name, const std::string &Value, const DateTime &Expires,
                                    const std::string &Path = "", const std::string &Domain = "",
                                    bool Secure = false, bool HttpOnly = false)
                {
                    // Expires=Thu, 21 Oct 2021 07:28:00 GMT;

                    std::stringstream ResultStream;

                    ResultStream << Name << "=" << Value;

                    ResultStream << "; Expires=" << Expires.ToGMT().Format(29, "%a, %d %b %Y %T %X GMT");

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

                Response &RemoveCookie(const std::string &Name)
                {
                    std::stringstream ResultStream;

                    ResultStream << Name << "=";

                    ResultStream << "; Max-Age=-1";

                    SetCookies.Add(ResultStream.str());

                    return *this;
                }

                size_t ParseFirstLine(std::string_view const &Text) override
                {
                    // @todo Hanlde null case

                    size_t Cursor = 5;
                    size_t CursorTmp = 0;

                    // Parse version

                    // @todo Maybe check the version's length?

                    if((CursorTmp = Text.find(' ', Cursor)) == std::string::npos)
                    {
                        throw std::invalid_argument("Invalid version");
                    }
                    
                    Version = Text.substr(Cursor, CursorTmp - Cursor);
                    Cursor = CursorTmp + 1;

                    // Parse code

                    // @todo Maybe check the code's length?

                    if((CursorTmp = Text.find(' ', Cursor)) == std::string::npos)
                    {
                        // @todo Optimize this
                        
                        throw std::invalid_argument("Invalid status code");
                    }

                    // @todo Optimize this
                    
                    Status = HTTP::Status(std::stoul(std::string(Text.substr(Cursor, CursorTmp - Cursor))));
                    Cursor = CursorTmp + 1;

                    // Parse breif

                    if((CursorTmp = Text.find('\r', Cursor)) == std::string::npos)
                    {
                        throw std::invalid_argument("Invalid breif");
                    }
                    
                    Brief = Text.substr(Cursor, CursorTmp - Cursor);

                    return CursorTmp + 2;
                }

                static Response From(std::string_view const &Text, size_t BodyIndex = 0)
                {
                    Response ret;
                    size_t Index = 0;

                    Index = ret.ParseFirstLine(Text);
                    Index = ret.ParseHeaders(Text, Index, BodyIndex);
                    ret.ParseContent(Text, Index);

                    return ret;
                }

                // @todo Optimize rvalue

                static Response From(std::string Version, HTTP::Status Status, std::map<std::string, std::string> Headers = {}, std::string Content = "")
                {
                    Response response;
                    response.Status = Status;
                    response.Version = std::move(Version);
                    response.Headers = std::move(Headers);
                    response.Content = std::move(Content);
                    response.Brief = StatusMessage.at(Status);
                    return response;
                }

                static Response Text(std::string Version, HTTP::Status Status, std::string Content)
                {
                    auto size = Content.size();
                    return From(std::move(Version), Status, {{"Content-Type", "text/plain"}, {"Content-Length", std::to_string(size)}}, std::move(Content));
                }

                static Response HTML(std::string Version, HTTP::Status Status, std::string Content)
                {
                    auto size = Content.size();
                    return From(std::move(Version), Status, {{"Content-Type", "text/html"}, {"Content-Length", std::to_string(size)}}, std::move(Content));
                }

                static Response Json(std::string Version, HTTP::Status Status, std::string Content)
                {
                    auto size = Content.size();
                    return From(std::move(Version), Status, {{"Content-Type", "application/json"}, {"Content-Length", std::to_string(size)}}, std::move(Content));
                }

                static inline Response Redirect(std::string Version, HTTP::Status Status, std::string Location, std::map<std::string, std::string> Parameters = {})
                {
                    std::stringstream str;

                    str << Location << '?';

                    for (auto &[key, value] : Parameters)
                    {
                        str << key << "=" << value << "&";
                    }

                    Parameters.insert_or_assign("Location", str.str());

                    return From(std::move(Version), Status, std::move(Parameters), "");
                }

                static inline Response Redirect(std::string Version, std::string Location, std::map<std::string, std::string> Parameters = {})
                {
                    return Redirect(std::move(Version), HTTP::Status::Found, std::move(Location), std::move(Parameters));
                }
            };
        }
    }
}