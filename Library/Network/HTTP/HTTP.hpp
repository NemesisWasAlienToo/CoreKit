#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <regex>

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

            std::map<Status, std::string> const StatusMessage{
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

            std::string const MethodStrings[]{"GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "TRACE", "CONNECT", "PATCH", ""};

            class Common
            {
            public:
                std::string Version;
                std::map<std::string, std::string> Headers;
                std::string Content;

                virtual std::string ToString() const = 0;
            };

            void applicationForm(const std::string &Message, std::map<std::string, std::string> &Result)
            {
                std::regex Capture("([^&]+)=([^&]+)");

                auto QueryBegin = std::sregex_iterator(Message.begin(), Message.end(), Capture);
                auto QueryEnd = std::sregex_iterator();

                std::smatch Matches;

                for (std::sregex_iterator i = QueryBegin; i != QueryEnd; ++i)
                {
                    std::cout << i->str(1) << " : " << i->str(2) << std::endl;
                    Result[i->str(1)] = i->str(2);
                }
            }

            void multipartForm(const std::string &Boundry, const std::string &Message, std::map<std::string, std::string> &Result)
            {
                std::regex Capture(Boundry + "\r\nContent-Disposition:\\s?form-data;\\s?name=\"([^\"]+)\"\r\n\r\n([^-\n]+)");

                auto QueryBegin = std::sregex_iterator(Message.begin(), Message.end(), Capture);
                auto QueryEnd = std::sregex_iterator();

                std::smatch Matches;

                for (std::sregex_iterator i = QueryBegin; i != QueryEnd; ++i)
                {
                    Result[i->str(1)] = i->str(2);
                }
            }

            void multipartForm(const std::string &Message, std::map<std::string, std::string> &Result)
            {
                std::regex Capture("name=\"([^\"]+)\"\r\n\r\n([^-\n]+)");

                auto QueryBegin = std::sregex_iterator(Message.begin(), Message.end(), Capture);
                auto QueryEnd = std::sregex_iterator();

                std::smatch Matches;

                for (std::sregex_iterator i = QueryBegin; i != QueryEnd; ++i)
                {
                    Result[i->str(1)] = i->str(2);
                }
            }

            void FormData(const Common &Message, std::map<std::string, std::string> &Result)
            {
                std::smatch Matches;

                auto Iterator = Message.Headers.find("Content-Type");

                if (Iterator == Message.Headers.end())
                {
                    return;
                }

                // @todo Optimize this later

                if(std::regex_match(Iterator->second, Matches, std::regex("application/x-www-form-urlencoded")))
                {
                    applicationForm(Message.Content, Result);
                }
                else if(std::regex_match(Iterator->second, Matches, std::regex("multipart/form-data; boundary=\"?([^\"]+)\"?")))
                {
                    auto BoundryString = Matches.str(1);
                    // multipartForm(BoundryString, Message.Content, Result);
                    multipartForm(Message.Content, Result);
                }
            }

        }
    }
}