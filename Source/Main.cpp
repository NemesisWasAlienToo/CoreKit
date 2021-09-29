#include <iostream>
#include <string>
#include <cstring>

#include "Iterable/List.cpp"

#include "Network/DNS.cpp"
#include "Network/Socket.cpp"
#include "Base/Poll.cpp"

int main(int argc, char const *argv[])
{
    std::cout << "Running on " << Network::DNS::HostName() << std::endl;

    auto result = Network::DNS::Resolve("google.com");

    Network::EndPoint Google(result[0], 80);

    std::cout << "Google is at " << Google << std::endl;

    Network::Socket client(Network::Socket::IPv4, Network::Socket::TCP);

    client.Connect(Google);

    client << "GET / HTTP/1.1 \r\n"
              "Host: ConfusionBox \r\n"
              "Connecttion: closed\r\n\r\n";

    for (client.Await(Network::Socket::In); client.HasData(); client.Await(Network::Socket::In))
    {
        std::cout << client;
    }

    client.Close();

    return 0;
}
