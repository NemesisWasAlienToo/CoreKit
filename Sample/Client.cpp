#include <iostream>
#include <string>
#include <cstring>

#include "Iterable/List.cpp"
#include "Network/DNS.cpp"
#include "Network/Socket.cpp"
#include "Base/Poll.cpp"
#include "Buffer/FIFO.cpp"

int main(int argc, char const *argv[])
{
    std::cout << "Running on " << Core::Network::DNS::HostName() << std::endl;

    auto result = Core::Network::DNS::Resolve("google.com");

    Core::Network::EndPoint Google(result[0], 80);

    std::cout << "Google is at " << Google << std::endl;

    Core::Network::Socket client(Core::Network::Socket::IPv4, Core::Network::Socket::TCP);

    client.Connect(Google);

    Core::Buffer::FIFO Buffer(1024);

    Buffer << "GET / HTTP/1.1\r\n"
              "Host: ConfusionBox\r\n"
              "Connecttion: closed\r\n\r\n";

    while (!Buffer.IsEmpty())
    {
        client << Buffer;

        client.Await(Core::Network::Socket::Out);
    }

    for (client.Await(Core::Network::Socket::In); client.Received() > 0; client.Await(Core::Network::Socket::In, 3000))
    {
        std::cout << client;
    }

    client.Close();

    return 0;
}
