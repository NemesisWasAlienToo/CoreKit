#pragma once

#include <iostream>
#include <string>
#include <mutex>
#include <functional>
#include <map>

#include "Test.cpp"
#include "Timer.cpp"
#include "DateTime.cpp"
#include "Format/Serializer.cpp"
#include "Network/EndPoint.cpp"
#include "Iterable/List.cpp"
#include "Network/DHT/Node.cpp"
#include "Network/DHT/Request.cpp"

using namespace Core;

namespace Core
{
    namespace Network
    {

        class Handler
        {
        public:
            typedef std::function<void()> EndCallback;
            typedef std::function<bool(const Iterable::Span<char> &)> RoutineCallback;

        private:
            std::map<Network::EndPoint, RoutineCallback> _Content;

        public:
            Handler() = default;
            ~Handler() = default;

            // Functionalities

            RoutineCallback Respond()
            {
                //
            }

            void Feed(const Network::EndPoint &Peer, const Iterable::Span<char> &Data)
            {
                if (_Content.contains(Peer))
                {
                    // If Handler exists

                    auto &Handler = _Content[Peer];

                    if (Handler(Data))
                    {
                        _Content.erase(Peer);
                    }
                }
                else
                {
                    // Build response handler

                    _Content.insert({Peer, Respond()});
                }
            }
        };

    }
}