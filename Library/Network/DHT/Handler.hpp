#pragma once

#include <iostream>
#include <string>
#include <mutex>
#include <functional>
#include <map>

#include "Test.hpp"
#include "Timer.hpp"
#include "DateTime.hpp"
#include "Format/Serializer.hpp"
#include "Network/EndPoint.hpp"
#include "Iterable/List.hpp"
#include "Network/DHT/DHT.hpp"
#include "Network/DHT/Node.hpp"

namespace Core
{
    namespace Network
    {
        namespace DHT
        {
            class Handler
            {
            public:
                typedef std::function<void(Node &, Format::Serializer &, EndCallback)> Callback;

                struct Handle
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

                // Should not be used brfore accuring the lock

                inline bool Has(const Network::EndPoint &EndPoint)
                {
                    return _Content.contains(EndPoint);
                }

                void Wind()
                {
                    if (_Content.size() != 0)
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
                        _Closest.second.Expire = DateTime::Now();
                        ExpireEvent.Stop();
                    }
                }

            public:
                Handler() = default;
                Handler(Core::Timer::TimerTypes type) : ExpireEvent(type, 0) {} // @todo Add default timeout
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
                    ExpireEvent.Value();

                    std::lock_guard<std::mutex> Lock(_Lock);

                    if (Has(_Closest.first)) // @todo Not needed
                    {
                        Network::EndPoint Item = _Closest.first;
                        auto &handle = _Content[Item];

                        if (handle.Expire <= DateTime::Now())
                        {
                            handle.End({Report::Codes::TimeOut});
                            _Content.erase(_Closest.first);
                        }
                    }

                    Wind();
                }

                bool Clean(std::function<void(const EndPoint &)> Callback)
                {
                    bool Ret = false;
                    Network::EndPoint EP;

                    ExpireEvent.Value();

                    {
                        std::lock_guard<std::mutex> Lock(_Lock);

                        if (Has(_Closest.first)) // @todo Not needed
                        {
                            Network::EndPoint Item = _Closest.first;
                            auto &handle = _Content[Item];

                            if ((Ret = handle.Expire <= DateTime::Now()))
                            {
                                handle.End({Report::Codes::TimeOut});
                                _Content.erase(_Closest.first);
                                EP = _Closest.first;
                            }
                        }

                        Wind();
                    }

                    if (Ret)
                    {
                        Callback(EP);
                    }

                    return Ret;
                }

                bool Put(const Network::EndPoint &EndPoint, const DateTime &Expire, const Callback &Routine, const EndCallback &End)
                {
                    // @todo THis function must return a pointer to the added item for std::function to be moved when adding a req
                    if (Expire <= DateTime::Now())
                        throw std::invalid_argument("expire time cannot be now or in the past");

                    std::lock_guard<std::mutex> Lock(_Lock);

                    if (Has(EndPoint))
                        return false;

                    _Content[EndPoint] = {Routine, Expire, End};

                    // New

                    if (_Closest.second.Expire <= DateTime::Now() || _Closest.second.Expire > Expire)
                    {
                        _Closest.first = EndPoint;
                        _Closest.second.Expire = Expire;

                        ExpireEvent.Set(_Closest.second.Expire.Left());
                    }

                    return true;
                }

                void Replace(const Network::EndPoint &EndPoint, const Callback &Routine, const DateTime &Expire)
                {
                    std::lock_guard<std::mutex> Lock(_Lock);

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

                bool Take(const Network::EndPoint &EndPoint, const std::function<void(Callback &, EndCallback &)> &After)
                {
                    bool Ret;
                    Callback CB;
                    EndCallback End;

                    {
                        std::lock_guard<std::mutex> Lock(_Lock);

                        if (!Has(EndPoint))
                        {
                            return false;
                        }

                        auto &handle = _Content[EndPoint];

                        if ((Ret = handle.Expire > DateTime::Now()))
                        {
                            CB = std::move(handle.Routine);
                            End = std::move(handle.End);
                        }
                        else
                        {
                            handle.End({Report::Codes::TimeOut});
                        }

                        _Content.erase(EndPoint);

                        Wind();
                    }

                    After(CB, End);

                    return Ret;
                }

                Handler &operator=(const Handler &Other) = default;
            };
        }
    }
}