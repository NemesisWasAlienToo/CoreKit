#include <iostream>
#include <string>

#include <Network/DNS.hpp>
#include <Network/Socket.hpp>
#include <Iterable/Queue.hpp>
#include <Network/HTTP/HTTP.hpp>
#include <Network/HTTP/Response.hpp>
#include <Network/HTTP/Request.hpp>
#include <Network/HTTP/Parser.hpp>

using namespace Core;

int main(int argc, char const *argv[])
{
    std::cout << "Running on " << Network::DNS::HostName() << std::endl;

    auto result = Network::DNS::Resolve("google.com");

    Network::EndPoint Google(result[0], 80);

    std::cout << "Google is at " << Google << std::endl
              << std::endl;

    Network::Socket Client(Network::Socket::IPv4, Network::Socket::TCP);

    Client.Connect(Google);

    Client.Await(Network::Socket::Out);

    Iterable::Queue<char> buffer;
    Format::Serializer ser(buffer);

    ser << "GET / HTTP/1.1 \r\n"
           "Host: ConfusionBox \r\n"
           "Content-Length: abc \r\n"
           "Connection: closed\r\n\r\n";

    while (!ser.Queue.IsEmpty())
    {
        Client << ser;
    }

    Network::HTTP::Parser parser;

    Client.Await(Network::Socket::In);

    parser.Start(Client);

    while (!parser.IsFinished())
    {
        Client.Await(Network::Socket::In);

        parser.Continue();
    }

    std::cout << parser.Result.ToString() << std::endl;

    Client.Close();

    return 0;
}