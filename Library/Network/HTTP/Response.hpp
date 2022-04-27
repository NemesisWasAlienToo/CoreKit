#pragma once

#include <iostream>
#include <string>
#include <map>

#include <Network/HTTP/HTTP.hpp>

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

                std::string ToString() const
                {
                    std::string str = "HTTP/" + Version + " " + std::to_string(static_cast<unsigned short>(Status)) + " " + Brief + "\r\n";
                    for (auto const &kv : Headers)
                        str += kv.first + ": " + kv.second + "\r\n";

                    return str + "\r\n" + Content;
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
                        Cursor = CursorTmp + 6;
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