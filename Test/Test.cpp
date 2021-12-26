#include <iostream>
#include <string>
#include <thread>
#include <functional>
#include <mutex>

#include <DateTime.cpp>
#include <Test.cpp>
#include <File.cpp>
#include <Timer.cpp>
#include <Directory.cpp>
#include <Iterable/List.cpp>
#include <Iterable/Queue.cpp>
#include <Iterable/Poll.cpp>
#include <Network/DNS.cpp>
#include <Network/Socket.cpp>
#include <Network/DHT/Server.cpp>
#include <Network/DHT/Handler.cpp>
#include <Network/DHT/Chord.cpp>

using namespace Core;

int main(int argc, char const *argv[])
{
    // Init EndPoint

    Network::EndPoint EndPoint("0.0.0.0:8888");

    // Init Key

    Network::DHT::Key Identity(32, 0);

    // Log End Point

    Test::Log(Network::DNS::HostName()) << EndPoint << std::endl;

    // Run the server

    Network::DHT::Chord::Runner Chord(Identity, EndPoint);

    // Chord.Bootstrap();

    Chord.Run();

    std::string _loop;

    while (_loop != "exit")
    {
        // Perform Route

        Chord.Route(
            Network::EndPoint("127.0.0.1:4444"),
            Identity,
            [](auto Result)
            {
                std::cout << "Routed to : " << Result << std::endl;
            });

        std::cout << "Waiting for commands, master" << std::endl;

        std::cin >> _loop;
    }

    Chord.Stop();

    return 0;
}