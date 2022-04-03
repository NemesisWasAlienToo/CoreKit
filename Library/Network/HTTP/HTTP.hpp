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
            enum class Status : unsigned short
            {
                Continue = 100,
                SwitchingProtocols = 101,
                OK = 200,
                Created = 201,
                Accepted = 202,
                NonAuthoritativeInformation = 203,
                NoContent = 204,
                ResetContent = 205,
                PartialContent = 206,
                MultipleChoices = 300,
                MovedPermanently = 301,
                Found = 302,
                SeeOther = 303,
                NotModified = 304,
                UseProxy = 305,
                TemporaryRedirect = 307,
                BadRequest = 400,
                Unauthorized = 401,
                PaymentRequired = 402,
                Forbidden = 403,
                NotFound = 404,
                MethodNotAllowed = 405,
                NotAcceptable = 406,
                ProxyAuthenticationRequired = 407,
                RequestTimeout = 408,
                Conflict = 409,
                Gone = 410,
                LengthRequired = 411,
                PreconditionFailed = 412,
                RequestEntityTooLarge = 413,
                RequestURITooLong = 414,
                UnsupportedMediaType = 415,
                RequestedRangeNotSatisfiable = 416,
                ExpectationFailed = 417,
                InternalServerError = 500,
                NotImplemented = 501,
                BadGateway = 502,
                ServiceUnavailable = 503,
                GatewayTimeout = 504,
                HTTPVersionNotSupported = 505
            };

            class Common
            {
            public:
                std::string Version;
                std::map<std::string, std::string> Headers;
                std::string Content;

                virtual std::string ToString() const = 0;

            protected:
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

                std::string ToString() const
                {
                    std::string str = Type + " " + Path + " HTTP/" + Version + "\r\n";
                    for (auto const &kv : Headers)
                        str += kv.first + ": " + kv.second + "\r\n";

                    str += "Content-Length: " + std::to_string(Content.length()) + "\r\n";

                    return str + "\r\n" + Content;
                }

                static Request From(const std::string &Text, size_t BodyIndex = 0)
                {
                    Request ret;

                    auto _Sepration = BodyIndex ? BodyIndex : Text.find("\r\n\r\n");
                    auto _HeadersText = Text.substr(0, _Sepration);
                    auto _ContentText = Text.substr(_Sepration + 4);

                    auto HeadersList = _SplitString(_HeadersText, "\r\n");

                    auto Info = std::move(HeadersList[0]);
                    HeadersList.Swap(0);

                    int Pos1 = Info.find(' ');

                    ret.Type = std::move(Info.substr(0, Pos1));

                    int Pos2 = Info.find(' ', ++Pos1);

                    ret.Path = Info.substr(Pos1, Pos2 - Pos1);

                    ret.Version = std::move(Info.substr(++Pos2 + 5));

                    HeadersList.ForEach(
                        [&](const std::string SingleHeader)
                        {
                            auto KeyValue = _SplitString(SingleHeader, ": ");
                            ret.Headers[KeyValue[0]] = KeyValue[1];
                        });

                    ret.Content = std::move(_ContentText);

                    ret.Headers["Content-Length"] = ret.Content.length();

                    return ret;
                }
            };

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

                static Response From(const std::string &Text)
                {
                    Response ret;

                    auto _Sepration = Text.find("\r\n\r\n");
                    auto _HeadersText = Text.substr(0, _Sepration);
                    auto _ContentText = Text.substr(_Sepration + 4);

                    auto HeadersList = _SplitString(_HeadersText, "\r\n");

                    auto Info = std::move(HeadersList[0]);
                    HeadersList.Swap(0);

                    int Pos1 = Info.find(' ');

                    ret.Version = Info.substr(5, Pos1 - 5);

                    int Pos2 = Info.find(' ', ++Pos1);

                    ret.Status = HTTP::Status(std::stoul(Info.substr(Pos1, Pos2 - Pos1)));

                    ret.Brief = Info.substr(++Pos2);

                    HeadersList.ForEach([&](const std::string &SingleHeader)
                                        {
                        auto KeyValue = _SplitString(SingleHeader, ": ");
                        ret.Headers[std::move(KeyValue[0])] = std::move(KeyValue[1]); });

                    ret.Content = std::move(_ContentText);

                    ret.Headers["Content-Length"] = ret.Content.length();

                    return ret;
                }
            };
        }
    }
}