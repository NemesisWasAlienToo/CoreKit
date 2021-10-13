#include <iostream>
#include <string>
#include <thread>

#include "Iterable/List.cpp"
#include "Network/DNS.cpp"
#include "Network/Address.cpp"
#include "Network/Socket.cpp"

void HandleClient(Core::Network::Socket Client, Core::Network::EndPoint Info)
{
    std::string Request;

    bool Condition = false;

    size_t Contet_Length = 0;

    while (!Condition)
    {
        Client.Await(Core::Network::Socket::In);

        if (Client.Received() <= 0)
            return;

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

    // std::string Content = "<body>hi there</body>";
    std::string Content = "{\"Name\" : \"Nemesis\"}";

    Buffer << "HTTP/1.1 200 OK\r\n"
              "Content-Type: text/plain\r\n"
            //   "Content-Type: text/html; charset=UTF-8\r\n"
              "Content-Type: application/json; charset=UTF-8\r\n"
              "Connection: closed\r\n"
              "Content-Length: " + std::to_string(Content.length()) + "\r\n\r\n"
              + Content;

    while (!Buffer.IsEmpty())
    {
        Client << Buffer;

        Client.Await(Core::Network::Socket::Out);
    }

    Client.Close();
}

int main(int argc, char const *argv[])
{
    Core::Network::EndPoint Host(Core::Network::Address(Core::Network::Address::Any()), 8888);

    Core::Network::Socket server(Core::Network::Socket::IPv4, Core::Network::Socket::TCP);

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
