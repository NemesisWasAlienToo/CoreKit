#pragma once

#include <iostream>
#include <string>
#include <thread>

#include "Network/EndPoint.cpp"
#include "Network/UDPServer.cpp"
#include "Iterable/List.cpp"
#include "Iterable/Span.cpp"
#include "Iterable/Poll.cpp"
#include "Duration.cpp"
#include "Test.cpp"

namespace Core
{
    namespace Network
    {
        class Runner
        {
        private:
            enum class States : char
            {
                Stopped = 0,
                Running,
            };

            Iterable::Poll Poll;
            Network::Server Server;

            States State;

        public:
            // Constructors

            Runner() = default;

            Runner(const EndPoint Peer) : Poll(1), Server(Peer), State(States::Stopped)
            {
                Poll.Add(Server.Listener(), Server.Events());

                // @todo Fix this

                Server.Builder = [this](const EndPoint &Peer, const Iterable::Span<char> &Data)
                {
                    constexpr size_t Padding = 8;

                    if (Data[0] != 'C' || Data[1] != 'H' || Data[2] != 'R' || Data[3] != 'D')
                    {
                        return;
                    }

                    // Get Size

                    uint32_t Lenght = ntohl(*(uint32_t *)(&Data[4])) - Padding;

                    // Build Queue

                    Iterable::Queue<char> Queue(Lenght, false);

                    Queue.Add(Data.Content() + Padding, Data.Length() - Padding);

                    if (Queue.IsFull())
                    {
                        Respond(Peer, Queue);
                    }
                    else
                    {
                        Server.Attach(
                            Peer,
                            [this, Peer, QU = std::move(Queue)](const Iterable::Span<char> &Data) mutable -> bool
                            {
                                QU.Add(Data.Content(), Data.Length());

                                if (QU.IsFull())
                                {
                                    Respond(Peer, QU);
                                    return true;
                                }
                                else
                                {
                                    return false;
                                }
                            });
                    }
                };
            }

            // Private functionalities

        private:
            void Respond(const EndPoint &Peer, Iterable::Queue<char> &Request)
            {
                Format::Serializer Serializer(Request);
                DHT::Node node(32);
                char Type;

                Serializer >> node.Id;

                Serializer >> Type;

                switch (Type)
                {
                case 2:
                {
                    std::cout << "Ping" << std::endl;

                    Iterable::Queue<char> Res;
                    Format::Serializer Ser(Res);

                    Server.Fire(
                        Peer,
                        [](Format::Serializer &Ser)
                        {
                            Ser << '\0';
                        });

                    break;
                }
                default:
                {
                    std::cout << "Other" << std::endl;
                }
                }
            }

            // Public functionalities

        public:
            void Run()
            {
                State = States::Running;

                while (State == States::Running)
                {
                    Poll[0].Mask = Server.Events();

                    Poll();

                    if (Poll[0].Happened(Iterable::Poll::Out))
                    {
                        Server.OnSend();
                    }

                    if (Poll[0].Happened(Iterable::Poll::In))
                    {
                        Server.OnReceive();
                    }
                }
            }
        };
    }
}