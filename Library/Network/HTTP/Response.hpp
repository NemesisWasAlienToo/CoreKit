#pragma once

#include <iostream>
#include <string>

#include <Duration.hpp>
#include <DateTime.hpp>
#include <Network/HTTP/HTTP.hpp>
#include <Iterable/List.hpp>
#include <Format/Stream.hpp>

namespace Core::Network::HTTP
{
    class Response : public Message
    {
    public:
        HTTP::Status Status;
        std::string Brief;
        Iterable::List<std::string> SetCookies;
        std::string Content;

        // friend Format::Stream &operator>>(Format::Stream &Ser, Response const &R)
        friend Format::Stream &operator<<(Format::Stream &Ser, Response const &R)
        {
            Ser << "HTTP/" << R.Version << ' ' << std::to_string(static_cast<unsigned short>(R.Status)) << ' ' << R.Brief << "\r\n";

            for (auto const &[k, v] : R.Headers)
                Ser << k << ": " << v << "\r\n";

            R.SetCookies.ForEach(
                [&Ser](std::string const &Cookie)
                {
                    Ser << "Set-Cookie: " << Cookie << "\r\n";
                });

            return Ser << "\r\n"
                       << R.Content;
        }

        std::string ToString() const
        {
            Iterable::Queue<char> Buffer;
            Format::Stream Ser(Buffer);

            Ser << *this;

            return std::string{&Buffer.Head(), Buffer.Length()};
        }

        Response &SetCookie(const std::string &Name, const std::string &Value,
                            const std::string &Path = "", const std::string &Domain = "",
                            bool Secure = false, bool HttpOnly = false)
        {
            // @todo Change stringstream to Format::Stream

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

        Response &RemoveCookie(const std::string &Name)
        {
            std::stringstream ResultStream;

            ResultStream << Name << "=";

            ResultStream << "; Max-Age=-1";

            SetCookies.Add(ResultStream.str());

            return *this;
        }

        size_t ParseFirstLine(std::string_view Text)
        {
            // @todo Hanlde null case

            size_t Cursor = 5;
            size_t CursorTmp = 0;

            // Parse version

            // @todo Maybe check the version's length?

            if ((CursorTmp = Text.find(' ', Cursor)) == std::string::npos)
            {
                throw std::invalid_argument("Invalid version");
            }

            Version = Text.substr(Cursor, CursorTmp - Cursor);
            Cursor = CursorTmp + 1;

            // Parse code

            // @todo Maybe check the code's length?

            if ((CursorTmp = Text.find(' ', Cursor)) == std::string::npos)
            {
                // @todo Optimize this

                throw std::invalid_argument("Invalid status code");
            }

            // @todo Optimize this

            Status = HTTP::Status(std::stoul(std::string(Text.substr(Cursor, CursorTmp - Cursor))));
            Cursor = CursorTmp + 1;

            // Parse brief

            if ((CursorTmp = Text.find('\r', Cursor)) == std::string::npos)
            {
                throw std::invalid_argument("Invalid brief");
            }

            Brief = Text.substr(Cursor, CursorTmp - Cursor);

            return CursorTmp + 2;
        }

        static Response From(std::string_view Text, size_t BodyIndex = 0)
        {
            Response ret;
            size_t Index = 0;

            Index = ret.ParseFirstLine(Text);
            Index = ret.ParseHeaders(Text, Index, BodyIndex - 4);
            ret.ParseContent(Text, Index);

            return ret;
        }

        static Response From(std::string_view Version, HTTP::Status Status, std::unordered_map<std::string, std::string> Headers = {}, std::string Content = "")
        {
            Response response;
            response.Status = Status;
            response.Brief = StatusMessage.at(Status);
            response.Version = Version;

            response.Headers = std::move(Headers);
            response.Content = std::move(Content);
            return response;
        }

        // String

        static Response Type(std::string_view Version, HTTP::Status Status, std::string_view ContentType, std::string Content = "")
        {
            return From(Version, Status, {{"Content-Type", std::string{ContentType}}}, std::move(Content));
        }

        static Response Text(std::string_view Version, HTTP::Status Status, std::string Content)
        {
            return Type(Version, Status, "text/plain", std::move(Content));
        }

        static Response HTML(std::string_view Version, HTTP::Status Status, std::string Content)
        {
            return Type(Version, Status, "text/html", std::move(Content));
        }

        static Response Json(std::string_view Version, HTTP::Status Status, std::string Content)
        {
            return Type(Version, Status, "application/json", std::move(Content));
        }

        // Redirect

        static inline Response Redirect(std::string_view Version, HTTP::Status Status, std::string_view Location, std::unordered_map<std::string, std::string> Parameters = {})
        {
            std::stringstream str;

            str << Location;

            Parameters.emplace("Location", str.str());

            return From(Version, Status, std::move(Parameters), "");
        }

        static inline Response Redirect(std::string_view Version, std::string_view Location, std::unordered_map<std::string, std::string> Parameters = {})
        {
            return Redirect(Version, HTTP::Status::Found, std::move(Location), std::move(Parameters));
        }
    };
}