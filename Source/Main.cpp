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
    // Init Identity

    const Network::EndPoint Target("192.168.1.17:4444");
    // const Network::EndPoint Target("127.0.0.1:4444");

    const Network::EndPoint EndPoint("0.0.0.0:8888");

    const Network::DHT::Key Key = Network::DHT::Key::Generate(4);

    const Network::DHT::Node Identity(Key, EndPoint); // <-- @todo Maybe add generate function?

    // Log End Point

    Test::Log(Network::DNS::HostName()) << EndPoint << std::endl;

    Test::Log("Identity") << Identity.Id.ToString() << std::endl;

    // Run the server

    // Network::DHT::Runner<Network::DHT::Chord> Chrod;

    Network::DHT::Chord::Runner Chord(Identity, {5, 0}, 1);

    Chord.Run();

    // Chord.Await(); // <--- Await all requests to be finished

    std::string Command = "";

    while (Command != "exit")
    {
        // Perform Route

        if (Command == "ping")
        {
            Chord.Ping(
                Target,
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
                Target,
                Identity.Id,
                [](Network::DHT::Node Result, Network::DHT::Handler::EndCallback End)
                {
                    Test::Log("Query") << Result.EndPoint << std::endl;
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
                Target,
                Identity.Id,
                [](Network::DHT::Node Result, Network::DHT::Handler::EndCallback End)
                {
                    Test::Log("Route") << Result.EndPoint << std::endl;
                    End();
                },
                []()
                {
                    Test::Log("Route") << "Ended" << std::endl;
                });
        }
        else if (Command == "boot")
        {
            Chord.Bootstrap(
                Target,
                [](std::function<void()> End)
                {
                    End();
                },
                []()
                {
                    std::cout << "Bootstrap ended" << std::endl;
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
        else if (Command == "print")
        {
            Chord.Nodes.ForEach(
                [](Network::DHT::Node &node)
                {
                    std::cout << node.Id.ToString() << std::endl;
                });
        }

        std::cout << "Waiting for command, master : " << std::endl;

        std::cin >> Command;
    }

    Chord.Stop();

    return 0;
}