#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <cstring>
#include <thread>
#include <chrono>
#include <mutex>
#include <functional>

#include "Iterable/List.cpp"
#include "Network/DNS.cpp"
#include "Network/Socket.cpp"
#include "Base/Poll.cpp"
#include "Cryptography/Digest.cpp"
#include "Cryptography/Random.cpp"

void HandleClient(Core::Network::Socket Client, Core::Network::EndPoint Info)
{
    std::string Request;

    bool Condition = false;

    size_t Contet_Length = 0;

    while (!Condition)
    {
        Client.Await(Core::Network::Socket::In);

        if(Client.Received() <= 0) return;

        Client >> Request;

        size_t pos = 0;

        if ((pos = Request.find("Content-Length: ")) != std::string::npos)
        {
            std::string len = Request.substr(pos + 16);
            len = len.substr(0, len.find("\r\n"));

            Contet_Length = std::stoul(len);
        }

        if ((pos = Request.find("\r\n\r\n")) != std::string::npos && Request.substr(pos + 4).length() >= Contet_Length)
            Condition = true;
    }

    std::cout << Info << " Says : " << std::endl
              << Request << std::endl;

    Core::Buffer::FIFO Buffer(1024);

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

        std::thread handler(HandleClient, Client, Info);

        handler.detach();
    }
}
