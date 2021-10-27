#include <iostream>
#include <string>
#include <cstring>
#include <streambuf>
#include "Iterable/List.cpp"
#include "Network/DNS.cpp"
#include "Network/Socket.cpp"
#include "Network/HTTP.cpp"

using namespace Core;
using namespace std;

int main(int argc, char const *argv[])
{
    cout << "Google is at " << "Running on " << Network::DNS::HostName() << endl;

    auto result = Network::DNS::Resolve("google.com");

    Network::EndPoint Google(result[0], 80);

    cout << "Google is at " << Google << endl;

    Network::Socket client(Network::Socket::IPv4, Network::Socket::TCP);

    client.Connect(Google);

    if(!client.IsConnected()) return -1;

    Iterable::Queue<char> Buffer(128);

    Network::HTTP::Request Req;

    Req.Type = "GET";
    Req.Version = "1.1";
    Req.Headers["Host"] = "ConfusionBox";
    Req.Headers["Connection"] = "closed";

    string requestText = Req.ToString();

    Buffer.Add(requestText.c_str(), requestText.length());

    // Send Request

    while (!Buffer.IsEmpty())
    {
        client << Buffer;

        client.Await(Network::Socket::Out);
    }

    // Receive Response

    for (client.Await(Network::Socket::In); client.Received() > 0; client.Await(Network::Socket::In, 3000))
    {
        client >> Buffer;
    }

    auto ResponseText = Buffer.ToString();

    Network::HTTP::Response response = Network::HTTP::Response::From(ResponseText);

    cout << response.ToString() << endl;

    client.Close();

    return 0;
}