#pragma once

#include <string>
#include <string_view>
#include <map>
#include <unordered_map>

#include <Duration.hpp>
#include <Iterable/Queue.hpp>

namespace Core
{
    namespace Network
    {
        namespace HTTP
        {
            constexpr auto HTTP10 = "1.0";
            constexpr auto HTTP11 = "1.1";

            // @todo Move this to response
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

            std::string const Message[]{
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

            std::unordered_map<Status, std::string> const StatusMessage{
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

            std::string const MethodStrings[]{"GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "PATCH", ""};

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

            // @todo Optimize this by maybe std::unordered_map?
            Methods FromString(std::string_view method)
            {
                if (method == "GET")
                    return Methods::GET;
                if (method == "POST")
                    return Methods::POST;
                if (method == "PUT")
                    return Methods::PUT;
                if (method == "DELETE")
                    return Methods::DELETE;
                if (method == "HEAD")
                    return Methods::HEAD;
                if (method == "OPTIONS")
                    return Methods::OPTIONS;
                if (method == "PATCH")
                    return Methods::PATCH;
                return Methods::Any;
            }

            class Message
            {
            public:
                std::string Version;
                std::unordered_map<std::string, std::string> Headers;
                std::string Content;

                // virtual bool IsValid() = 0;
                // virtual std::string ToString() const = 0;
                // virtual size_t ParseFirstLine(std::string_view const &Text) = 0;

                size_t ParseHeaders(std::string_view const &Text, size_t Start, size_t End = 0)
                {
                    size_t Cursor = Start;
                    size_t CursorTmp = 0;
                    size_t BodyStart = End;

                    if (End == 0)
                    {
                        BodyStart = Text.find("\r\n\r\n");
                        BodyStart = BodyStart == std::string::npos ? Text.length() : BodyStart;
                    }

                    while (Cursor < BodyStart && (CursorTmp = Text.find(':', Cursor)) < BodyStart)
                    {
                        // Find key

                        std::string HeaderKey(Text.substr(Cursor, CursorTmp - Cursor));
                        Cursor = Text[CursorTmp + 1] == ' ' ? CursorTmp + 2 : CursorTmp + 1;

                        // Find value

                        CursorTmp = Text.find('\r', Cursor);

                        if (CursorTmp == std::string::npos)
                        {
                            std::string HeaderValue(Text.substr(Cursor));
                            Headers.insert_or_assign(std::move(HeaderKey), HeaderValue);

                            break;
                        }

                        std::string HeaderValue(Text.substr(Cursor, CursorTmp - Cursor));
                        Cursor = CursorTmp + 2;

                        Headers.insert_or_assign(std::move(HeaderKey), std::move(HeaderValue));
                    }

                    return BodyStart + 4;
                }

                void ParseContent(const std::string_view &Text, size_t BodyIndex)
                {
                    Content = std::string{Text.substr(BodyIndex)};
                }
            };
        }
    }
}