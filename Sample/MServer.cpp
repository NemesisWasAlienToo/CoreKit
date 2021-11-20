#include <iostream>
#include <string>
#include <thread>

#include "Iterable/List.cpp"
#include "Iterable/Queue.cpp"
#include "Network/DNS.cpp"
#include "Network/Address.cpp"
#include "Network/Socket.cpp"
#include "Network/HTTP.cpp"

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
}

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

    Core::Network::HTTP::Request Req = Core::Network::HTTP::Request::From(Request);

    std::cout << Info << " Says : " << std::endl << Req.ToString() << std::endl;

    Core::Iterable::Queue<char> Buffer(1024);

    Core::Network::HTTP::Response Res;

    Res.Version = "1.1";
    Res.Status = 200;
    Res.Brief = "OK";
    Res.Headers["Host"] = "ConfusionBox";
    Res.Headers["Connection"] = "closed";
    Res.Headers["Content-Type"] = "text/html; charset=UTF-8";
    // Res.Headers["Content-Type"] = "text/plain";
    // Res.Headers["Content-Type"] = "application/json; charset=UTF-8";
    // Res.Headers["Content-Type"] = "application/json";

    Res.Body = "<h1>Hi</h1>";

    std::string ResponseText = Res.ToString();
    Buffer.Add(ResponseText.c_str(), ResponseText.length());

    while (!Buffer.IsEmpty())
    {
        Client << Buffer;

        Client.Await(Core::Network::Socket::Out);
    }

    Client.Close();
}