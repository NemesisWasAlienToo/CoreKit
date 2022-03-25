#include <iostream>
#include <string>
#include <cstring>

#include <Iterable/List.hpp>
#include <Network/DNS.hpp>
#include <Network/Socket.hpp>
#include <Iterable/Queue.hpp>
#include <Iterable/Poll.hpp>
#include <Format/Serializer.hpp>

using namespace Core;

int main(int argc, char const *argv[])
{
    std::cout << "Running on " << Network::DNS::HostName() << std::endl;

    auto result = Network::DNS::Resolve("google.com");

    Network::EndPoint Google(result[0], 80);

    std::cout << "Google is at " << Google << std::endl;

    Network::Socket client(Network::Socket::IPv4, Network::Socket::TCP);

    client.Connect(Google);

    client.Await(Network::Socket::Out);

    Iterable::Queue<char> buffer;
    Format::Serializer ser(buffer);

    ser << "GET / HTTP/1.1 \r\n"
           "Host: ConfusionBox \r\n"
           "Connecttion: closed\r\n\r\n";

    while (!ser.Queue.IsEmpty())
    {
        client << ser;
    }

    ser.Clear();

    client.Await(Network::Socket::Out);

    for (client.Await(Network::Socket::In); client.Received() > 0; client.Await(Network::Socket::In))
    {
        client >> ser;
    }

    std::cout << ser;

    client.Close();

    return 0;
}
