#include <iostream>
#include <string>
#include <functional>

#include <Test.cpp>
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

    Network::DHT::Chord::Runner Chord(Identity, EndPoint, 1, 10);

    // Known Node

    Network::DHT::Node KnownNode = Network::DHT::Node(Network::DHT::Key("f0f00f", 32), Network::EndPoint("127.0.0.1:4444"));

    Chord.Add(KnownNode);

    Chord.Run();

    // Chord.Bootstrap();

    // Chord.Await(); <--- Await all requests to be finished

    std::string Command = "";

    while (Command != "exit")
    {
        // Perform Route

        if (Command == "ping")
        {
            Chord.Ping(
                KnownNode.EndPoint,
                [](Duration Result, Network::DHT::Handler::EndCallback End)
                {
                    Test::Log("Ping") << Result << "s" << std::endl;
                },
                []()
                {
                    Test::Log("Ping") << "Ended" << std::endl;
                });
        }
        else if (Command == "query")
        {
            Chord.Query(
                Network::EndPoint("127.0.0.1:4444"),
                Identity,
                [](Network::EndPoint Result, Network::DHT::Handler::EndCallback End)
                {
                    Test::Log("Query") << Result << std::endl;
                    End();
                },
                []()
                {
                    Test::Log("Query") << "Ended" << std::endl;
                });
        }
        else if (Command == "route")
        {
            Chord.Route(
                Identity,
                [](Network::EndPoint Result, Network::DHT::Handler::EndCallback End)
                {
                    Test::Log("Route") << Result << std::endl;
                    End();
                },
                []()
                {
                    Test::Log("Route") << "Ended" << std::endl;
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