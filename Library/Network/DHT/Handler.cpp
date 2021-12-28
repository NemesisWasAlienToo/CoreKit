#pragma once

#include <iostream>
#include <string>
#include <mutex>
#include <functional>
#include <map>

#include "Test.cpp"
#include "Timer.cpp"
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
                typedef std::function<void(bool)> EndCallback;

                struct Handle // @todo Add move semmantics
                {
                    Callback Routine;
                    DateTime Expire;
                };

            private:
                std::mutex _Lock;
                std::map<Network::EndPoint, Handle> _Content;

                const Callback &_Default;

                std::pair<Network::EndPoint, Handle> _Closest;
                Timer ExpireEvent;

                // Shall not be used brfore accuring the lock

                inline bool Has(const Network::EndPoint &EndPoint)
                {
                    return _Content.contains(EndPoint);
                }

            public:
                Handler(const Callback &DefaultHandler) : _Default(DefaultHandler), ExpireEvent(Timer::Monotonic, 0) {}
                ~Handler() = default;

                // Functionalities

                std::pair<Network::EndPoint, Handle> Soonest()
                {
                    if (_Content.size() <= 0)
                        throw std::out_of_range("Instance contains no handler");

                    std::pair<Network::EndPoint, Handle> Result = *_Content.begin();

                    for (auto Item : _Content)
                    {
                        if (Item.second.Expire < Result.second.Expire)
                        {
                            Result = Item;
                        }
                    }

                    return Result;
                }

                inline Descriptor Listener()
                {
                    return ExpireEvent.INode();
                }

                void Clean()
                {
                    ExpireEvent.Value();

                    std::lock_guard Lock(_Lock);

                    if (Has(_Closest.first))
                    {
                        auto &handle = _Content[_Closest.first];

                        if (handle.Expire <= DateTime::Now())
                        {
                            _Content.erase(_Closest.first);
                        }
                    }

                    if (_Content.size() > 0)
                    {
                        _Closest = Soonest();

                        Duration Left;

                        if (_Closest.second.Expire > DateTime::Now())
                        {
                            Left = _Closest.second.Expire.Left();
                            // Left.AddMilliseconds(100);
                        }
                        else
                        {
                            Left = Duration(0, 1);
                        }

                        ExpireEvent.Set(Left);
                    }
                    else
                    {
                        ExpireEvent.Set({0, 0});
                    }
                }

                bool Put(const Network::EndPoint &EndPoint, const DateTime &Expire, const Callback &Routine)
                {
                    if (Expire <= DateTime::Now())
                        throw std::invalid_argument("expire time cannot be now or in the past");

                    std::lock_guard Lock(_Lock);

                    if (Has(EndPoint))
                        return false;

                    _Content[EndPoint] = {Routine, Expire};

                    // New

                    if (_Closest.second.Expire < Expire)
                    {
                        _Closest.first = EndPoint;
                        _Closest.second.Expire = Expire;

                        auto Left = _Closest.second.Expire.Left();
                        
                        ExpireEvent.Set(Left);
                    }

                    return true;
                }

                void Replace(const Network::EndPoint &EndPoint, const Callback &Routine, const DateTime &Expire)
                {
                    std::lock_guard Lock(_Lock);

                    _Content[EndPoint] = {Routine, Expire};

                    // New

                    if (_Closest.second.Expire < Expire)
                    {
                        _Closest.first = EndPoint;
                        _Closest.second.Expire = Expire;

                        auto Left = _Closest.second.Expire.Left();

                        ExpireEvent.Set(Left);
                    }
                }

                bool Take(const Network::EndPoint &EndPoint, Callback &Routine)
                {
                    // @todo Optimize access

                    std::lock_guard Lock(_Lock);

                    if (!Has(EndPoint))
                    {
                        Routine = _Default;

                        return false;
                    }

                    auto &handle = _Content[EndPoint];

                    if (handle.Expire <= DateTime::Now())
                    {
                        _Content.erase(EndPoint);

                        Routine = _Default;
                        return false;
                    }

                    Routine = std::move(handle.Routine);

                    _Content.erase(EndPoint);

                    return true;
                }

                Handler &operator=(Handler &Other) = default;
            };
        }
    }
}