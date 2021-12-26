// # Standard headers

#include <iostream>
#include <string>
#include <mutex>

// # User headers

#include "Iterable/List.cpp"
#include "DateTime.cpp"
#include "Timer.cpp"
#include "File.cpp"
#include "DynamicLib.cpp"
#include <Network/DNS.cpp>
#include <Network/Socket.cpp>

// # Usings

using namespace Core;

// # Global variables

#define MESSAGE "Hello there people"
#define LEN (8 + sizeof(MESSAGE))

int main(int argc, char const *argv[])
{
    Network::EndPoint Host(Network::Address::Any(Network::Address::IPv4), 8888);

    Network::Socket client(Network::Socket::IPv4, Network::Socket::UDP);

    char Signiture[8] = {'C', 'H', 'R', 'D', 0, 0, 0, 0};

    std::string Message;

    std::cin >> Message;

    auto len = Message.length() + 1;

    *(uint32_t *)(&Signiture[4]) = htonl(len + sizeof(Signiture));

    client.SendTo(Signiture, sizeof(Signiture), Host);

    sleep(1);

    client.SendTo(Message.c_str(), len, Host);

    return 0;
}