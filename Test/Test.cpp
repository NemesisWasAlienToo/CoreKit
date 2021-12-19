#include <iostream>
#include <string>
#include <thread>
#include <functional>
#include <map>

#include <Test.cpp>
#include <File.cpp>
#include <Timer.cpp>
#include <Directory.cpp>
#include <Iterable/List.cpp>
#include <Iterable/Queue.cpp>
#include <Iterable/Poll.cpp>
#include <Network/DNS.cpp>
#include <Network/Socket.cpp>

using namespace Core;

// Handlers

// @todo Needs a mutex

Iterable::List<std::function<void(void *)>> Handlers;

// Request Queue

// @todo Needs a mutex

Iterable::Queue<Iterable::Queue<char>> Requests;

// @todo Thread pool

//

// tasks

struct Request
{
    Iterable::Queue<char> Buffer;
    Network::EndPoint Peer;
    unsigned char Ready = false;
};

void Query(Network::EndPoint Peer, std::string Id, std::function<void(Network::EndPoint)> Callback)
{
    // Write packet

    std::cout << Id << std::endl;

    // Add Handler

    Handlers.Add([Callback](void *Args)
                 {
        // Cast Argus

        std::string& Respons = *(std::string *)Args;

        // Build result from response

        Network::EndPoint Result;

        // Delete Response

        delete &Respons;

        // Execute Callback

        Callback(Result); });
}

int main(int argc, char const *argv[])
{
    Iterable::List<Request> Sends;
    Iterable::List<Request> Receives;

    Network::EndPoint Host(Network::Address::Any(Network::Address::IPv4), 8888);

    Network::Socket server(Network::Socket::IPv4, Network::Socket::UDP);

    server.Bind(Host);

    std::cout << Network::DNS::HostName() << " is listenning on " << Host << std::endl;

    Network::Socket::Event Events = Network::Socket::In;

    while (true)
    {
        Network::EndPoint Target;

        if (Sends.Length() != 0)
            Events |= Network::Socket::Out;

        auto Event = server.Await(Events);

        if (Event & Network::Socket::In)
        {
            // Read identifier

            char Identifier[8];

            server.ReceiveFrom(Identifier, 8, Target, Network::Socket::Peek);

            // Check if it has pending request
            Receives.Where([Target](Request &Item) -> bool
                           { return true; });

            if (true)
            {
                //
            }
            else
            {
                Iterable::Queue<char> Buffer;

                //
            }
        }

        if (Event & Network::Socket::Out)
        {
            //
        }
    }

    return 0;
}