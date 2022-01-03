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
                typedef std::function<void()> EndCallback;
                typedef std::function<void(Network::DHT::Request &, EndCallback)> Callback;

                struct Handle // @todo Add move semmantics
                {
                    Callback Routine;
                    DateTime Expire;
                    EndCallback End;
                };

            private:
                std::mutex _Lock;
                std::map<Network::EndPoint, Handle> _Content;

                std::pair<Network::EndPoint, Handle> _Closest;
                Timer ExpireEvent;

                // Shall not be used brfore accuring the lock

                inline bool Has(const Network::EndPoint &EndPoint)
                {
                    return _Content.contains(EndPoint);
                }

            public:
                Handler() : ExpireEvent(Timer::Monotonic, 0) {} // @todo Add default timeout
                ~Handler() = default;

                // Functionalities

                std::pair<Network::EndPoint, Handle> Soonest()
                {
                    if (_Content.size() <= 0)
                        throw std::out_of_range("Instance contains no handler");

                    // @todo Optimize by keeping a refrence

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
                    // @todo return expired item
                    
                    ExpireEvent.Value();

                    std::lock_guard Lock(_Lock);

                    if (Has(_Closest.first))
                    {
                        auto &handle = _Content[_Closest.first];

                        if (handle.Expire <= DateTime::Now())
                        {
                            handle.End();
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

                bool Put(const Network::EndPoint &EndPoint, const DateTime &Expire, const Callback &Routine, const EndCallback &End)
                {
                    // @todo THis function must return a pointer to the added item for std::function to be moved when adding a req
                    if (Expire <= DateTime::Now())
                        throw std::invalid_argument("expire time cannot be now or in the past");

                    std::lock_guard Lock(_Lock);

                    if (Has(EndPoint))
                        return false;

                    _Content[EndPoint] = {Routine, Expire, End};

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

                bool Take(const Network::EndPoint &EndPoint, Callback &Routine, EndCallback &End)
                {
                    // @todo Optimize access

                    std::lock_guard Lock(_Lock);

                    if (!Has(EndPoint))
                    {
                        return false;
                    }

                    auto &handle = _Content[EndPoint];

                    bool Ret;

                    if ((Ret = handle.Expire <= DateTime::Now()))
                    {
                        handle.End();

                        _Content.erase(EndPoint);
                    }
                    else
                    {
                        Routine = std::move(handle.Routine);
                        End = std::move(handle.End);

                        _Content.erase(EndPoint);
                    }

                    if (_Content.size() > 0)
                    {
                        _Closest = Soonest();

                        Duration Left;

                        if (_Closest.second.Expire > DateTime::Now())
                        {
                            Left = _Closest.second.Expire.Left();
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

                    return !Ret;
                }

                Handler &operator=(Handler &Other) = default;
            };
        }
    }
}