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

    const Network::DHT::Key Key = Network::DHT::Key::Generate(4);

    const Network::DHT::Node Identity(Key, EndPoint); // <-- @todo Maybe add generate function?

    // Log End Point

    Test::Log(Network::DNS::HostName()) << EndPoint << std::endl;

    Test::Log("Identity") << Identity.Id.ToString() << std::endl;

    // Run the server

    Network::DHT::Chord::Runner Chord(Identity, {5, 0}, 1);

    Chord.Add({{"0000000f"}, {"127.0.0.1:0"}});
    Chord.Add({{"000000f1"}, {"127.0.0.1:1"}});
    Chord.Add({{"00000ff2"}, {"127.0.0.1:2"}});
    Chord.Add({{"0000fff3"}, {"127.0.0.1:3"}});
    Chord.Add({{"000ffff4"}, {"127.0.0.1:4"}});
    Chord.Add({{"00fffff5"}, {"127.0.0.1:5"}});
    Chord.Add({{"0ffffff6"}, {"127.0.0.1:6"}});
    Chord.Add({{"fffffff7"}, {"127.0.0.1:7"}});

    Chord.Run();

    // Chord.Await(); // <--- Await all requests to be finished

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
        else if (Command == "boot")
        {
            Chord.Bootstrap(
                {"127.0.0.1:4444"},
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