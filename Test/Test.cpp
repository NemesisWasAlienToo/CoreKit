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

    const Network::EndPoint EndPoint("0.0.0.0:8888");

    const Network::DHT::Key Key = Network::DHT::Key::Generate(32);

    const Network::DHT::Node Identity(Key, EndPoint); // <-- @todo Maybe add generate function?

    // Log End Point

    Test::Log(Network::DNS::HostName()) << EndPoint << std::endl;

    Test::Log("Identity") << Identity.Id.ToString() << std::endl;

    // Run the server

    Network::DHT::Chord::Runner Chord(Identity, 1, 10);

    Chord.Run();

    std::string BootstrapDomain;

    std::cout << "Enter a bootstrap domain : ";

    std::cin >> BootstrapDomain;

    // Chord.Bootstrap(
    //     BootstrapDomain,
    //     [](std::function<void()> End)
    //     {
    //         End();
    //     },
    //     []()
    //     {
    //         std::cout << "Routing ended" << std::endl;
    //     });

    // Chord.Await(); <--- Await all requests to be finished

    std::string Command = "";

    while (Command != "exit")
    {
        // Perform Route

        if (Command == "ping")
        {
            Chord.Ping(
                {"127.0.0.1:4444"},
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
                {"127.0.0.1:4444"},
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
                {"127.0.0.1:4444"},
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