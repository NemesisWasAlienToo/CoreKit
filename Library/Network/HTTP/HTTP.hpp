#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <map>

#include "Iterable/List.hpp"

namespace Core
{
    namespace Network
    {
        namespace HTTP
        {
            class Common
            {
            public:
                std::string Version;
                std::map<std::string, std::string> Headers;
                std::string Body;

                virtual std::string ToString() const { return ""; };

            protected:
                Common() = default;
                Common(const Common &Other) : Version(Other.Version), Headers(Other.Headers), Body(Other.Body) {}
                Common(Common &&Other) : Version(std::move(Other.Version)), Headers(std::move(Other.Headers)), Body(std::move(Other.Body)) {}
                ~Common() {}

                static Iterable::List<std::string> _SplitString(const std::string &Text, const std::string &Seprator)
                {
                    Iterable::List<std::string> Temp;
                    size_t start = 0;
                    size_t end;
                    const size_t skip = Seprator.length();

                    while (true)
                    {
                        std::string Sub;

                        if ((end = Text.find(Seprator, start)) == std::string::npos)
                        {
                            if (!(Sub = Text.substr(start)).empty())
                            {
                                Temp.Add(Sub);
                            }

                            break;
                        }

                        Sub = Text.substr(start, end - start);
                        Temp.Add(Sub);
                        start = end + skip;
                    }

                    return Temp;
                }
            };

            class Request : public Common
            {
            public:
                std::string Type;
                std::string Path;

                Request() = default;
                ~Request() {}

                std::string ToString() const
                {
                    std::string str = Type + " " + Path + " HTTP/" + Version + "\r\n";
                    for (auto const &kv : Headers)
                        str += kv.first + ": " + kv.second + "\r\n";

                    str += "Content-Length: " + std::to_string(Body.length()) + "\r\n";

                    return str + "\r\n" + Body;
                }

                static Request From(const std::string &Text)
                {
                    Request ret;

                    auto _Sepration = Text.find("\r\n\r\n");
                    auto _HeadersText = Text.substr(0, _Sepration);
                    auto _BodyText = Text.substr(_Sepration + 4);

                    auto HeadersList = _SplitString(_HeadersText, "\r\n");

                    auto Info = std::move(HeadersList[0]);
                    HeadersList.Swap(0);

                    int Pos1 = Info.find(' ');

                    ret.Type = std::move(Info.substr(0, Pos1));

                    int Pos2 = Info.find(' ', ++Pos1);

                    ret.Path = Info.substr(Pos1, Pos2 - Pos1);

                    ret.Version = std::move(Info.substr(++Pos2 + 5));

                    HeadersList.ForEach([&](const std::string SingleHeader)
                                        {
                        auto KeyValue = _SplitString(SingleHeader, ": ");
                        ret.Headers[KeyValue[0]] = KeyValue[1]; });

                    ret.Body = std::move(_BodyText);

                    ret.Headers["Content-Length"] = ret.Body.length();

                    return ret;
                }
            };

            class Response : public Common
            {
            public:
                unsigned short Status;
                std::string Brief;

                Response() = default;
                Response(const Response &Other) : Common(Other), Status(Other.Status), Brief(Other.Brief) {}
                Response(Response &&Other) : Common(std::move(Other)), Status(std::move(Other.Status)), Brief(std::move(Other.Brief)) {}
                ~Response() {}

                std::string ToString() const
                {
                    std::string str = "HTTP/" + Version + " " + std::to_string(Status) + " " + Brief + "\r\n";
                    for (auto const &kv : Headers)
                        str += kv.first + ": " + kv.second + "\r\n";

                    return str + "\r\n" + Body;
                }

                static Response From(const std::string &Text)
                {
                    Response ret;

                    auto _Sepration = Text.find("\r\n\r\n");
                    auto _HeadersText = Text.substr(0, _Sepration);
                    auto _BodyText = Text.substr(_Sepration + 4);

                    auto HeadersList = _SplitString(_HeadersText, "\r\n");

                    auto Info = std::move(HeadersList[0]);
                    HeadersList.Swap(0);

                    int Pos1 = Info.find(' ');

                    ret.Version = Info.substr(5, Pos1 - 5);

                    int Pos2 = Info.find(' ', ++Pos1);

                    ret.Status = std::stoul(Info.substr(Pos1, Pos2 - Pos1));

                    ret.Brief = Info.substr(++Pos2);

                    HeadersList.ForEach([&](const std::string &SingleHeader)
                                        {
                        auto KeyValue = _SplitString(SingleHeader, ": ");
                        ret.Headers[std::move(KeyValue[0])] = std::move(KeyValue[1]); });

                    ret.Body = std::move(_BodyText);

                    ret.Headers["Content-Length"] = ret.Body.length();

                    return ret;
                }
            };
        }
    }
}