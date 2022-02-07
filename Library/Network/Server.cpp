#pragma once

#include <iostream>
#include <string>
#include <map>
#include <mutex>
#include <thread>
#include <functional>

#include <Test.cpp>
#include <Event.cpp>

#include <Network/EndPoint.cpp>
#include <Network/Socket.cpp>

#include <Iterable/List.cpp>
#include <Iterable/Span.cpp>
#include <Iterable/Poll.cpp>

#include <Format/Serializer.cpp>
#include <Network/DHT/Node.cpp>

namespace Core
{
    namespace Network
    {
        class Server
        {
        public:
            enum class States : uint8_t
            {
                Running,
                Stopped,
            };

            typedef std::function<void()> EndCallback;
            typedef std::function<bool()> OutgoingCallback;
            typedef std::function<bool(const Iterable::Span<char> &)> IncommingCallback;
            typedef std::function<void(const EndPoint &, const Iterable::Span<char> &)> BuilderCallback;

            // template <typename T>
            // struct Entry
            // {
            //     DateTime Expire;
            //     EndCallback End;
            //     T Callback;
            // };

            // typedef Entry<OutgoingCallback> OutEntry;
            // typedef Entry<IncommingCallback> InEntry;

        private:
            Network::Socket _Socket;
            Iterable::Queue<OutgoingCallback> _Outgoing;
            std::map<Network::EndPoint, IncommingCallback> _Incomming;

        public:
            // Variables
            
            BuilderCallback Builder;

            // Constructors

            Server() = default;

            Server(const Network::EndPoint &EndPoint) : _Socket(Network::Socket::IPv4, Network::Socket::UDP), _Outgoing(1)
            {
                _Socket.Bind(EndPoint);
            }

            // Functionalities

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

                if (Last())
                {
                    _Outgoing.Take();
                }
            }

            void OnReceive()
            {
                auto [Data, Peer] = _Socket.ReceiveFrom();

                if (!_Incomming.contains(Peer))
                {
                    Builder(Peer, Data);
                }
                else
                {
                    if (_Incomming[Peer](Data))
                    {
                        // If its done

                        _Incomming.erase(Peer);
                    }
                }
            }

            Iterable::Queue<char> Prepare(const std::function<void(Format::Serializer &)> &Builder, size_t Size = 1024)
            {
                Iterable::Queue<char> request(Size + 9);

                Format::Serializer Serializer(request);

                Serializer.Add((char *)"CHRD", 4) << (uint32_t)0;

                Builder(Serializer);

                Serializer.Modify<uint32_t>(4) = Format::Serializer::Order((uint32_t)request.Length());

                return request;
            }

            inline void Fire(const Network::EndPoint &Peer, const std::function<void(Format::Serializer &)> &Builder)
            {
                auto Request = Prepare(
                    [this, &Builder](Format::Serializer &Ser)
                    {
                        // Fill in my id for me

                        Ser << DHT::Key::Generate(4);

                        // Build

                        Builder(Ser);
                    });

                _Outgoing.Add(
                    [this, Peer, QU = std::move(Request)]() mutable -> bool
                    {
                        // @todo Can still improve this

                        QU.Free(_Socket.SendTo(QU.Content(), QU.Chunk(), Peer));

                        return QU.IsEmpty();
                    });
            }

            bool Attach(const Network::EndPoint &Peer, const IncommingCallback &Callback)
            {
                if (_Incomming.contains(Peer))
                {
                    return false;
                }

                _Incomming.insert({Peer, Callback});
                return true;
            }

            bool Attach(const Network::EndPoint &Peer, IncommingCallback &&Callback)
            {
                if (_Incomming.contains(Peer))
                {
                    return false;
                }

                _Incomming.insert({Peer, std::move(Callback)});
                return true;
            }

            void Replace(const Network::EndPoint &Peer, const IncommingCallback &Callback)
            {
                _Incomming[Peer] = Callback;
            }

            void Replace(const Network::EndPoint &Peer, IncommingCallback &&Callback)
            {
                _Incomming[Peer] = std::move(Callback);
            }
        };
    }
}
