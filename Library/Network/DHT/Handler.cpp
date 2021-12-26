#pragma once

#include <iostream>
#include <string>
#include <mutex>
#include <functional>
#include <map>

#include "Test.cpp"
#include "DateTime.cpp"
#include "Network/EndPoint.cpp"
#include "Iterable/List.cpp"

#include "Network/DHT/Request.cpp"

using namespace Core;

namespace Core
{
    namespace Network
    {
        namespace DHT
        {
            class Handler
            {
            public:
                typedef std::function<void(Network::DHT::Request &)> Callback;

                struct Handle
                {
                    Callback Routine;
                    DateTime Expire;
                };

            private:
                std::mutex _Lock;
                std::map<Network::EndPoint, Handle> _Content;

                const Callback& _Default;

                // Shall not be used brfore accuring the lock

                inline bool Has(const Network::EndPoint &EndPoint)
                {
                    return _Content.contains(EndPoint);
                }

            public:
                Handler(const Callback& DefaultHandler) : _Default(DefaultHandler) {}
                ~Handler() = default;

                // Functionalities

                bool Put(const Network::EndPoint &EndPoint, const DateTime &Expire, const Callback &Routine)
                {
                    std::lock_guard Lock(_Lock);

                    if (Has(EndPoint))
                        return false;

                    Handle handle
                    {
                        .Routine = Routine,
                        .Expire = Expire,
                    };

                    _Content[EndPoint] = std::move(handle);

                    return true;
                }

                void Replace(const Network::EndPoint &EndPoint, const Callback &Routine, const DateTime &Expire)
                {
                    std::lock_guard Lock(_Lock);

                    Handle handle{
                        .Routine = Routine,
                        .Expire = Expire,
                    };

                    _Content[EndPoint] = std::move(handle);
                }

                bool Take(const Network::EndPoint &EndPoint, Callback &Routine)
                {
                    std::lock_guard Lock(_Lock);

                    if (!Has(EndPoint))
                    {
                        Routine = _Default;

                        return false;
                    }

                    auto &handle = _Content[EndPoint];

                    if (handle.Expire.Epired())
                    {
                        _Content.erase(EndPoint);

                        Routine = _Default;
                        return false;
                    }

                    Routine = std::move(handle.Routine);

                    _Content.erase(EndPoint);

                    return true;
                }

                Handler& operator=(Handler& Other) = default;
            };
        }
    }
}