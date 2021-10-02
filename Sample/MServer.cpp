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

void HandleClient(Core::Network::Socket Client, Core::Network::EndPoint Info)
{
    Core::Buffer::FIFO Buffer(1024);

    // Buffer << "GET / HTTP/1.1\r\n"
    //           "Host: ConfusionBox\r\n"
    //           "Connecttion: closed\r\n\r\n";

    Buffer << "HTTP/1.1 200 OK\r\n"
              "Content-Type: text/plain\r\n"
              "Connection: closed\r\n"
              "Content-Length: 11\r\n\r\n"
              "hello there";

    while (!Buffer.IsEmpty())
    {
        Client << Buffer;

        Client.Await(Core::Network::Socket::Out);
    }

    Client.Close();
}

void LunchHandler(Core::Network::Socket &Client, Core::Network::EndPoint &Info)
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
    Core::Network::Socket server(Core::Network::Socket::IPv4, Core::Network::Socket::TCP);

    auto result = Core::Network::DNS::Host(Core::Network::Address::IPv4);

    Core::Network::EndPoint Host(result[0], 8888);

    server.Bind(Host);

    std::cout << Core::Network::DNS::HostName() << " is listenning on " << Host << std::endl;

    server.Listen(10);

    while (1)
    {
        Core::Network::EndPoint Info;

        auto Client = server.Accept(Info);

        LunchHandler(Client, Info);
    }
}
