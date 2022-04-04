#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <map>

#include <Duration.hpp>
#include <Iterable/List.hpp>
#include <Iterable/Queue.hpp>
#include <Network/DNS.hpp>
#include <Network/Socket.hpp>
#include <Format/Serializer.hpp>

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

            const std::string Message[]{
                "Continue",
                "Switching Protocols",
                "OK",
                "Created",
                "Accepted",
                "Non-Authoritative Information",
                "No Content",
                "Reset Content",
                "Partial Content",
                "Multiple Choices",
                "Moved Permanently",
                "Found",
                "See Other",
                "Not Modified",
                "Use Proxy",
                "Temporary Redirect",
                "Bad Request",
                "Unauthorized",
                "Payment Required",
                "Forbidden",
                "Not Found",
                "Method Not Allowed",
                "Not Acceptable",
                "Proxy Authentication Required",
                "Request Timeout",
                "Conflict",
                "Gone",
                "Length Required",
                "Precondition Failed",
                "Request Entity Too Large",
                "Request-URI Too Long",
                "Unsupported Media Type",
                "Requested Range Not Satisfiable",
                "Expectation Failed",
                "Internal Server Error",
                "Not Implemented",
                "Bad Gateway",
                "Service Unavailable",
                "Gateway Timeout",
                "HTTP Version Not Supported"};

            const std::map<Status, std::string> StatusMessage{
                {Status::Continue, "Continue"},
                {Status::SwitchingProtocols, "Switching Protocols"},
                {Status::OK, "OK"},
                {Status::Created, "Created"},
                {Status::Accepted, "Accepted"},
                {Status::NonAuthoritativeInformation, "Non-Authoritative Information"},
                {Status::NoContent, "No Content"},
                {Status::ResetContent, "Reset Content"},
                {Status::PartialContent, "Partial Content"},
                {Status::MultipleChoices, "Multiple Choices"},
                {Status::MovedPermanently, "Moved Permanently"},
                {Status::Found, "Found"},
                {Status::SeeOther, "See Other"},
                {Status::NotModified, "Not Modified"},
                {Status::UseProxy, "Use Proxy"},
                {Status::TemporaryRedirect, "Temporary Redirect"},
                {Status::BadRequest, "Bad Request"},
                {Status::Unauthorized, "Unauthorized"},
                {Status::PaymentRequired, "Payment Required"},
                {Status::Forbidden, "Forbidden"},
                {Status::NotFound, "Not Found"},
                {Status::MethodNotAllowed, "Method Not Allowed"},
                {Status::NotAcceptable, "Not Acceptable"},
                {Status::ProxyAuthenticationRequired, "Proxy Authentication Required"},
                {Status::RequestTimeout, "Request Timeout"},
                {Status::Conflict, "Conflict"},
                {Status::Gone, "Gone"},
                {Status::LengthRequired, "Length Required"},
                {Status::PreconditionFailed, "Precondition Failed"},
                {Status::RequestEntityTooLarge, "Request Entity Too Large"},
                {Status::RequestURITooLong, "Request-URI Too Long"},
                {Status::UnsupportedMediaType, "Unsupported Media Type"},
                {Status::RequestedRangeNotSatisfiable, "Requested Range Not Satisfiable"},
                {Status::ExpectationFailed, "Expectation Failed"},
                {Status::InternalServerError, "Internal Server Error"},
                {Status::NotImplemented, "Not Implemented"},
                {Status::BadGateway, "Bad Gateway"},
                {Status::ServiceUnavailable, "Service Unavailable"},
                {Status::GatewayTimeout, "Gateway Timeout"},
                {Status::HTTPVersionNotSupported, "HTTP Version Not Supported"}};

            class Common
            {
            public:
                std::string Version;
                std::map<std::string, std::string> Headers;
                std::string Content;

                virtual std::string ToString() const = 0;
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

                    return str + "\r\n" + Content;
                }

                static Request From(const std::string &Method, const std::string &Path, const std::string &Version = "1.1", const std::map<std::string, std::string> &Headers = {}, const std::string &Content = "")
                {
                    Request request;
                    request.Type = Method;
                    request.Path = Path;
                    request.Version = Version;
                    request.Headers = Headers;
                    request.Content = Content;
                    return request;
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

                static Request From(const std::string &Method, const std::string &Path, const std::string &Content)
                {
                    return Request::From(
                        Method,
                        Path,
                        "1.1",
                        {{"Host", Network::DNS::HostName()}, {"Connection", "closed"}, {"Content-Length", std::to_string(Content.size())}},
                        Content);
                }
                
                static Request Get(const std::string &Path)
                {
                    return Request::From(
                        "GET",
                        Path,
                        "1.1",
                        {{"Host", Network::DNS::HostName()}, {"Connection", "closed"}},
                        "");
                }

                static Request Post(const std::string &Path, const std::string &Content)
                {
                    return Request::From(
                        "POST",
                        Path,
                        "1.1",
                        {{"Host", Network::DNS::HostName()}, {"Connection", "closed"}, {"Content-Length", std::to_string(Content.size())}},
                        Content);
                }

                static Request Put(const std::string &Path, const std::string &Content)
                {
                    return Request::From(
                        "PUT",
                        Path,
                        "1.1",
                        {{"Host", Network::DNS::HostName()}, {"Connection", "closed"}, {"Content-Length", std::to_string(Content.size())}},
                        Content);
                }

                static Request Delete(const std::string &Path)
                {
                    return Request::From(
                        "DELETE",
                        Path,
                        "1.1",
                        {{"Host", Network::DNS::HostName()}, {"Connection", "closed"}},
                        "");
                }

                static Request Options(const std::string &Path)
                {
                    return Request::From(
                        "OPTIONS",
                        Path,
                        "1.1",
                        {{"Host", Network::DNS::HostName()}, {"Connection", "closed"}},
                        "");
                }

                static Request Head(const std::string &Path)
                {
                    return Request::From(
                        "HEAD",
                        Path,
                        "1.1",
                        {{"Host", Network::DNS::HostName()}, {"Connection", "closed"}},
                        "");
                }

                static Request Trace(const std::string &Path)
                {
                    return Request::From(
                        "TRACE",
                        Path,
                        "1.1",
                        {{"Host", Network::DNS::HostName()}, {"Connection", "closed"}},
                        "");
                }

                static Request Connect(const std::string &Path)
                {
                    return Request::From(
                        "CONNECT",
                        Path,
                        "1.1",
                        {{"Host", Network::DNS::HostName()}, {"Connection", "closed"}},
                        "");
                }

                static Request Patch(const std::string &Path, const std::string &Content)
                {
                    return Request::From(
                        "PATCH",
                        Path,
                        "1.1",
                        {{"Host", Network::DNS::HostName()}, {"Connection", "closed"}, {"Content-Length", std::to_string(Content.size())}},
                        Content);
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

                static Response From(HTTP::Status Status, const std::string &Version = "1.1", const std::map<std::string, std::string> &Headers = {}, const std::string &Content = "")
                {
                    Response response;
                    response.Status = Status;
                    response.Version = Version;
                    response.Headers = Headers;
                    response.Content = Content;
                    response.Brief = StatusMessage.at(Status);
                    return response;
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
            };

            Response Send(const EndPoint Target, const Request &Request, Duration TimeOut = {0, 0})
            {
                Network::Socket client(Network::Socket::IPv4, Network::Socket::TCP);

                client.Connect(Target);

                client.Await(Network::Socket::Out);

                Iterable::Queue<char> buffer;
                Format::Serializer Ser(buffer);

                Ser << Request.ToString();

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
        }
    }
}