#include <iostream>
#include <string>
#include <functional>

#include <Test.cpp>
#include <DateTime.cpp>
#include <Iterable/List.cpp>
#include <Iterable/Queue.cpp>
#include <Network/DNS.cpp>
#include <Network/DHT/Server.cpp>
#include <Network/DHT/Handler.cpp>
#include <Network/DHT/Chord.cpp>

using namespace Core;

int main(int argc, char const *argv[])
{
    Network::EndPoint Host{"0.0.0.0:4444"};

    Network::Socket server(Network::Socket::IPv4, Network::Socket::UDP);

    server.Bind(Host);

    std::cout << Network::DNS::HostName() << " is listenning on " << Host << std::endl;

    while (1)
    {
        Network::EndPoint Target;

        server.Await(Network::Socket::In);

        auto size = server.Received();

        char Buffer[size + 1];

        Buffer[size] = 0;

        server.ReceiveFrom(Buffer, sizeof(Buffer), Target);

        std::cout << Target << " : " << Buffer << std::endl;
    }

    return 0;
}