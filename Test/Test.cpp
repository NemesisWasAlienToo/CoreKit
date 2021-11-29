#include <iostream>
#include <string>
#include <thread>
#include <functional>

#include "Base/Test.cpp"
#include "Base/File.cpp"
#include "Iterable/List.cpp"
#include "Iterable/Queue.cpp"
#include "Cryptography/Digest.cpp"
#include "Network/DNS.cpp"
#include "Network/DHT/Chord.cpp"

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

    return 0;
}