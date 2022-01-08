#include <iostream>
#include <string>
#include <functional>

#include <Test.cpp>
#include <DateTime.cpp>
#include <Iterable/List.cpp>
#include <Iterable/Span.cpp>
#include <Network/DNS.cpp>
#include <Network/DHT/Server.cpp>
#include <Network/DHT/Handler.cpp>
#include <Network/DHT/Chord.cpp>

using namespace Core;

int main(int argc, char const *argv[])
{
    // Init Identity

    const Network::EndPoint Target("5.123.18.87:8888");

    const Network::EndPoint EndPoint("0.0.0.0:8888");

    const Network::DHT::Key Key = Network::DHT::Key::Generate(4);

    const Network::DHT::Node Identity(Key, EndPoint);

    // Log End Point

    Test::Log(Network::DNS::HostName()) << EndPoint << std::endl;

    Test::Log("Identity") << Identity.Id.ToString() << std::endl;

    // Run the server

    Network::DHT::Chord::Runner Chord(Identity, {5, 0}, 1);

    Chord.OnSet =
        [](const Core::Network::DHT::Key &Key, const Core::Iterable::Span<char> &Data)
    {
        Test::Log("Set") << "{ " << Key << ", " << Data << " }" << std::endl;
    };

    Chord.OnGet =
        [&Chord](const Core::Network::DHT::Key &Key, Network::DHT::Chord::Runner::OnGetCallback CB)
    {
        Test::Log("Get") << "{ " << Key << " }" << std::endl;

        Iterable::Span<char> Data("Get Received", 12);

        CB(Data);
    };

    Chord.OnData =
        [](Core::Iterable::Span<char> &Data)
    {
        Test::Log("Data") << Data << std::endl;
    };

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
                Network::DHT::Key::Generate(4),
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
                Network::DHT::Key::Generate(4),
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
            std::string Data = "Hello there";
            Chord.Set(
                Network::DHT::Key::Generate(4),
                {Data.c_str(), Data.length()},
                []()
                {
                    std::cout << "Set ended" << std::endl;
                });
        }
        else if (Command == "get")
        {
            Chord.Get(
                Network::DHT::Key::Generate(4),
                [](Iterable::Span<char> &Data, std::function<void()> End)
                {
                    Test::Log("Get") << "{ " << Data << " }" << std::endl;
                    End();
                },
                []()
                {
                    std::cout << "Get ended" << std::endl;
                });
        }
        else if (Command == "data")
        {
            std::string Message;

            std::cout << "Enter data : ";

            std::cin >> Message; 

            Chord.SendTo(Target, {Message.c_str(), Message.length()});
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