#include <iostream>
#include <string>
#include <functional>

#include <Test.cpp>
#include <Timer.cpp>
#include <DateTime.cpp>
#include <Iterable/List.cpp>
#include <Network/DNS.cpp>
#include <Network/DHT/Server.cpp>
#include <Network/DHT/Handler.cpp>
#include <Network/DHT/Chord.cpp>

using namespace Core;

int main(int argc, char const *argv[])
{
    // Init EndPoint

    const Network::EndPoint EndPoint("0.0.0.0:8888");

    // Init Key

    Network::DHT::Key Identity(Network::DHT::Key::Generate(32));

    // Log End Point

    Test::Log(Network::DNS::HostName()) << EndPoint << std::endl;

    Test::Log("Identity") << Identity.ToString() << std::endl;

    // Run the server

    Network::DHT::Chord::Runner Chord(Identity, EndPoint, 1, 60);

    // Known Node

    Network::DHT::Node KnownNode = Network::DHT::Node(Network::DHT::Key("f0f00f", 32), Network::EndPoint("127.0.0.1:4444"));

    Chord.AddNode(KnownNode);

    // Chord.Bootstrap();

    Chord.Run();

    std::string Command = "";

    while (Command != "exit")
    {
        // Perform Route

        if (Command == "ping")
        {
            Chord.Ping(
                KnownNode.EndPoint,
                [](Duration Result)
                {
                    Test::Log("Ping") << Result << "s" << std::endl;
                });
        }
        else if (Command == "query")
        {
            Chord.Query(
                Network::EndPoint("127.0.0.1:4444"),
                Identity,
                [](Network::EndPoint Result)
                {
                    Test::Log("Query") << Result << std::endl;
                });
        }
        else if (Command == "route")
        {
            Chord.Route(
                Identity,
                [](Network::EndPoint Result)
                {
                    Test::Log("Routed") << Result << std::endl;
                });
        }
        else if (Command == "set")
        {
            //
        }
        else if (Command == "get")
        {
            //
        }

        std::cout << "Waiting for command, master : " << std::endl;

        std::cin >> Command;
    }

    Chord.Stop();

    return 0;
}