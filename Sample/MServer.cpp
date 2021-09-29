#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <chrono>
#include <mutex>

#include "Iterable/List.cpp"

#include "Network/DNS.cpp"
#include "Network/Socket.cpp"

#include "Base/Poll.cpp"

void HandleClient(Network::Socket Client, Network::EndPoint Info)
{
    std::string str;

    std::cout << Info << " is now connected" << std::endl;

    // Client >> std::cout;

    Client << "Say something : ";

    Client >> str;

    Client << "You said : " << str;

    std::cout << "Client said : " << str;

    Client.Close();
}

void LunchHandler(Network::Socket &Client, Network::EndPoint &Info)
{
    static int Count = 0;

    if (Count++ > 10)
    {
        std::cout << Info << " is now connected" << std::endl;
        return;
    }

    std::thread handler(HandleClient, std::move(Client), std::move(Info));

    handler.detach();
}

int main(int argc, char const *argv[])
{
    Network::Socket server(Network::IPv4Protocol, Network::TCP, true);

    auto result = Network::DNS::Host(Network::IPv4Address);

    Network::EndPoint Host(result[0], 8888);

    server.Bind(Host);

    std::cout << Network::DNS::HostName() << " is listenning on " << Host << std::endl;

    server.Listen(10);

    while (1)
    {
        Network::EndPoint Info;

        auto Client = server.Accept(Info);

        LunchHandler(Client, Info);
    }
}
