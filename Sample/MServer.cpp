#include <iostream>
#include <string>
#include <thread>

#include "Iterable/List.hpp"
#include "Iterable/Queue.hpp"
#include "Network/DNS.hpp"
#include "Network/Address.hpp"
#include "Network/Socket.hpp"
#include "Network/HTTP/HTTP.hpp"
#include "Network/HTTP/Response.hpp"
#include "Network/HTTP/Request.hpp"
#include "Format/Serializer.hpp"

using namespace Core;

void HandleClient(Core::Network::Socket Client, Core::Network::EndPoint Info);

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

    server.Close();
}

void HandleClient(Core::Network::Socket Client, Core::Network::EndPoint Info)
{
    std::string Request;

    bool Condition = false;

    size_t Contet_Length = 0;

    while (!Condition)
    {
        Client.Await(Network::Socket::In);

        size_t Received = Client.Received();

        if (Received == 0)
        {
            return;
        }

        char RequestBuffer[Received];

        Client.Receive(RequestBuffer, Received);

        Request.append(RequestBuffer, Received);

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

    Core::Network::HTTP::Request Req = Core::Network::HTTP::Request::From(Request);

    std::cout << Info << " Says : " << std::endl
              << Req.ToString() << std::endl;

    Core::Iterable::Queue<char> Buffer(1024);

    Core::Network::HTTP::Response Res = Core::Network::HTTP::Response::HTML(Core::Network::HTTP::Status::OK, "<h1>Hi</h1>");

    std::string ResponseText = Res.ToString();
    Buffer.Add(ResponseText.c_str(), ResponseText.length());

    Format::Serializer Ser(Buffer);

    while (!Buffer.IsEmpty())
    {
        Client << Ser;

        Client.Await(Core::Network::Socket::Out);
    }

    Client.Close();
}