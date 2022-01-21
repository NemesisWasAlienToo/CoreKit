/**
 * @file Server.cpp
 * @author Nemesis (github.com/NemesisWasAlienToo)
 * @brief
 * @todo Add time out for _Incomming and out going requests
 * @version 0.1
 * @date 2022-01-11
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

#include <iostream>
#include <string>
#include <mutex>
#include <thread>
#include <functional>

#include <Test.cpp>
#include <Event.cpp>

#include <Network/EndPoint.cpp>
#include <Network/Socket.cpp>

#include <Iterable/List.cpp>
#include <Iterable/Poll.cpp>

#include <Format/Serializer.cpp>
#include <Network/DHT/Request.cpp>
#include <Network/Handler.cpp>

namespace Core
{
    namespace Network
    {
        class Server
        {
        private:
            Network::Socket _Socket;
            Iterable::Queue<Network::DHT::Request> _Outgoing;
            Network::Handler _Handler;

        public:
            enum class States : uint8_t
            {
                Running,
                Stopped,
            };

            Server() = default;

            Server(const Network::EndPoint &EndPoint) : _Socket(Network::Socket::IPv4, Network::Socket::UDP), _Outgoing(1)
            {
                _Socket.Bind(EndPoint);
            }

            inline Descriptor Listener()
            {
                return _Socket.INode();
            }

            Iterable::Poll::Event Events()
            {
                return _Outgoing.IsEmpty() ? (Iterable::Poll::In) : (Iterable::Poll::In | Iterable::Poll::Out);
            }

            void OnSend()
            {
                if (_Outgoing.IsEmpty())
                {
                    return;
                }

                auto &Last = _Outgoing.Last();

                Last.Buffer.Free(_Socket.SendTo(Last.Buffer.Content(), Last.Buffer.Length(), Last.Peer));

                if (Last.Buffer.IsEmpty())
                {
                    _Outgoing.Take();
                }
            }

            void OnReceive()
            {
                // Setup buffers

                size_t len = _Socket.Received();
                Iterable::Span<char> Data(len);
                Network::EndPoint Peer;

                // Read arrived data

                len = _Socket.ReceiveFrom(Data.Content(), len, Peer);

                _Handler.Feed(Peer, Data);
            }
        };
    }
}
