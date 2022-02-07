#pragma once

#include "string"

#include "Network/DHT/Node.cpp"
#include "Iterable/List.cpp"
#include "Iterable/Span.cpp"
#include "Duration.cpp"

namespace Core
{
    namespace Network
    {
        namespace DHT
        {
            enum class Operations : char
            {
                Response = 0,
                Invalid,
                Ping,
                Query,
                Keys,
                Set,
                Get,
                Data,
            };

            struct Report
            {
                enum class Codes : size_t
                {
                    Normal = 0,
                    Occupied,
                    TimeOut,
                    InvalidResponse,
                    InvalidArgument,
                };

                Codes Code;

                std::string Reason;

                static const std::string ErrorStrings[];

                static inline const std::string &ErrorString(Codes er)
                {
                    return ErrorStrings[static_cast<size_t>(er)];
                }

                Report() = default;
                Report(Codes _Code) : Code(_Code), Reason(ErrorString(_Code)) {}
                Report(Codes _Code, const std::string &_Reason) : Code(_Code), Reason(_Reason) {}
                Report(bool _HasError, Codes _Code, const std::string &_Reason) : Code(_Code), Reason(_Reason) {}

                inline bool HasError() const
                {
                    return static_cast<bool>(Code);
                }
            };

            const std::string Report::ErrorStrings[] =
                {
                    "Normal termination",
                    "EndPoint is occupied",
                    "EndPoint has timed out",
                    "EndPoint's response is invalid",
                    "Provided argument is invalid",
            };

            typedef std::function<void(const Iterable::List<Key> &)> OnKeysCallback;
            typedef std::function<void(Iterable::Span<char> &)> OnGetCallback;

            typedef std::function<void(const Report &)> EndCallback;
            template<typename TRes>
            using SuccessCallback = std::function<void(TRes, EndCallback)>;

            // @todo Optimize arguments by passing with refrence
            
            typedef SuccessCallback<Duration> PingCallback;
            typedef SuccessCallback<Iterable::List<Node>> QueryCallback;
            typedef SuccessCallback<Iterable::List<Node>> RouteCallback;
            typedef SuccessCallback<Iterable::List<Key> &> KeysCallback;
            typedef SuccessCallback<Iterable::Span<char> &> GetCallback;
            typedef SuccessCallback<Iterable::Span<char> &> SendToCallback;
        }
    }
}