#include <iostream>
#include <string>

#include <Machine.hpp>
#include <Network/Socket.hpp>
#include <Iterable/Queue.hpp>
#include <Network/HTTP/HTTP.hpp>
#include <Network/HTTP/Response.hpp>
#include <Network/HTTP/Request.hpp>

using namespace Core;

int main(int argc, char const *argv[])
{
    std::cout << "Running on " << Network::DNS::HostName() << std::endl;

    auto result = Network::DNS::Resolve("google.com");

    Network::EndPoint Google(result[0], 80);

    std::cout << "Google is at " << Google << std::endl << std::endl;

    Network::Socket client(Network::Socket::IPv4, Network::Socket::TCP);

    client.Connect(Google);

    client.Await(Network::Socket::Out);

    Iterable::Queue<char> buffer;
    Format::Serializer ser(buffer);

    ser << "GET / HTTP/1.1 \r\n"
           "Host: ConfusionBox \r\n"
           "Connection: closed\r\n\r\n";

    while (!ser.Queue.IsEmpty())
    {
        client << ser;
    }

    Network::HTTP::Parser p;

    client.Await(Network::Socket::In);

    p.Start(client);

    for (client.Await(Network::Socket::In); !p.IsFinished() && client.Received() > 0; client.Await(Network::Socket::In))
    {
        p.Continue();
    }

    auto Response = p.Parse<Network::HTTP::Response>();

    std::cout << Response.ToString() << std::endl;

    client.Close();

    return 0;
}