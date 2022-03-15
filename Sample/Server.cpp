#include <iostream>
#include <string>
#include <cstring>

#include "Iterable/List.hpp"

#include "Network/DNS.hpp"
#include "Network/Socket.hpp"

#include "Iterable/Poll.hpp"

using namespace Core;

int main(int argc, char const *argv[])
{
    Network::Socket server(Network::Socket::IPv4, Network::Socket::TCP);

    auto result = Network::DNS::Host(Network::Address::IPv4);

    Network::EndPoint Host(result[0], 8888);

    server.Bind(Host);

    std::cout << Network::DNS::HostName() << " is listenning on " << Host << std::endl;

    server.Listen(10);

    while (1)
    {
        Network::EndPoint Info;

        std::string str;

        auto Client = server.Accept(Info);

        // Client >> std::cout;

        Client << "Say something : ";

        Client >> str;

        Client << "You said : " << str;

        std::cout << "Client said : " << str;

        Client.Close();
    }
}
