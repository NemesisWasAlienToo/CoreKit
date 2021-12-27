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

    const Network::EndPoint EndPoint("0.0.0.0:8888");

    // Init Key

    Network::DHT::Key Identity("ff", 32);

    // Log End Point

    Test::Log(Network::DNS::HostName()) << EndPoint << std::endl;

    // Run the server

    Network::DHT::Chord::Runner Chord(Identity, EndPoint, 1, 60);

    // Chord.Bootstrap();

    Chord.Run();

    std::string Command = "";

    while (Command != "exit")
    {
        // Perform Route

        if(Command == "ping")
        {
            Chord.Ping(
                Network::EndPoint("127.0.0.1:4444"),
                [](double Result)
                {
                    Test::Log("Ping") << Result << "s" << std::endl;
                });
        }
        else if(Command == "query")
        {
            Chord.Query(
                Network::EndPoint("127.0.0.1:4444"),
                Identity,
                [](Network::EndPoint Result)
                {
                    Test::Log("Query") << Result << std::endl;
                });
        }
        else if(Command == "route")
        {
            Chord.Route(
                Network::EndPoint("127.0.0.1:4444"),
                Identity,
                [](Network::EndPoint Result)
                {
                    Test::Log("Routed") << Result << std::endl;
                });
        }
        else if(Command == "set")
        {
            //
        }
        else if(Command == "get")
        {
            //
        }

        std::cout << "Waiting for command, master : " << std::endl;

        std::cin >> Command;
    }

    Chord.Stop();

    return 0;
}