#include <iostream>
#include <string>
#include <cstring>
#include <streambuf>
#include "Iterable/List.cpp"
#include "Iterable/Buffer.cpp"
#include "Network/DNS.cpp"
#include "Network/Socket.cpp"
#include "Network/HTTP.cpp"
#include "Cryptography/Digest.cpp"
#include "Cryptography/Random.cpp"
#include "Base/Test.cpp"
#include "Iterable/Buffer.cpp"

using namespace Core;
using namespace std;

int main(int argc, char const *argv[])
{
    Test::Log("Running on " + Network::DNS::HostName());

    auto result = Network::DNS::Resolve("google.com");

    Network::EndPoint Google(result[0], 80);

    std::cout << "Google is at " << Google << std::endl;

    Network::Socket client(Network::Socket::IPv4, Network::Socket::TCP);

    client.Connect(Google);

    Iterable::Buffer<char> Buffer(128);

    Network::HTTP::Request Req;

    Req.Type = "GET";
    Req.Version = "1.1";
    Req.Headers["Host"] = "ConfusionBox";
    Req.Headers["Connection"] = "closed";

    std::string requestText = Req.ToString();

    Buffer.Add(requestText.c_str(), requestText.length());

    while (!Buffer.IsEmpty())
    {
        client << Buffer;

        client.Await(Network::Socket::Out);
    }

    for (client.Await(Network::Socket::In); client.Received() > 0; client.Await(Network::Socket::In, 3000))
    {
        std::cout << client;
    }

    client.Close();

    return 0;
}