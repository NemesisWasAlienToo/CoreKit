#include <iostream>
#include <string>
#include <functional>

#include <Test.hpp>
#include <File.hpp>
#include <DateTime.hpp>
#include <Iterable/List.hpp>
#include <Iterable/Span.hpp>
#include <Network/DNS.hpp>
#include <Network/DHT/Chord.hpp>
#include <Network/DHT/Runner.hpp>

using namespace Core;

int main(int argc, char const *argv[])
{
    // Init Identity

    const Network::DHT::Node Identity(Cryptography::Key::Generate(4), {"0.0.0.0:8888"});

    // Log End Point

    Test::Log(Network::DNS::HostName()) << Identity.EndPoint << std::endl;

    Test::Log("Identity") << Identity.Id.ToString() << std::endl;

    // Setup a target

    // std::string TargetString;

    // STDOUT << "Enter target ip : ";

    // STDIN >> TargetString;

    // const Network::EndPoint Target(TargetString);

    const Network::EndPoint Target("127.0.0.1:4444");

    // Setup the server

    Network::DHT::Runner<Network::DHT::Chord> Chord(Identity, {5, 0}, 1);

    Chord.OnKeys =
        [](Network::DHT::OnKeysCallback CB)
        {
            Iterable::List<Cryptography::Key> Keys;
            
            CB(Keys);
        };

    Chord.OnSet =
        [](/*const Network::DHT::Node Peer,*/ const Core::Cryptography::Key &Key, const Core::Iterable::Span<char> &Data)
        {
            Test::Log("Set") << "{ " << Key << ", " << Data << " }" << std::endl;
        };

    Chord.OnGet =
        [&Chord](/*const Network::DHT::Node Peer,*/ const Core::Cryptography::Key &Key, Network::DHT::OnGetCallback CB)
        {
            Test::Log("Get") << "{ " << Key << " }" << std::endl;

            Iterable::Span<char> Data("Get Received", 12);

            CB(Data);
        };

    Chord.OnData =
        [](const Network::DHT::Node Peer, Iterable::Span<char> &Data)
        {
            Test::Log("Data") << Data << std::endl;
        };

    // Run the server

    Chord.Run();

    STDOUT.WriteLine("Waiting for commands");

    std::string Command = STDIN.ReadLine();

    while (Command != "exit")
    {
        if (Command == "ping")
        {
            Chord.Ping(
                Target,
                [](Duration Result, Network::DHT::EndCallback End)
                {
                    Test::Log("Ping") << Result << "s" << std::endl;
                },
                [](const Network::DHT::Report& Report)
                {
                    Test::Log("Ping") << "Ended" << std::endl;
                });
        }
        else if (Command == "query")
        {
            Chord.Query(
                Target,
                Cryptography::Key::Generate(4),
                [](Iterable::List<Network::DHT::Node> Result, Network::DHT::EndCallback End)
                {
                    Test::Log("Query") << Result[0].EndPoint << std::endl;
                    End({Network::DHT::Report::Codes::Normal});
                },
                [](const Network::DHT::Report& Report)
                {
                    Test::Log("Query") << "Ended" << std::endl;
                });
        }
        else if (Command == "route")
        {
            Chord.Route(
                Cryptography::Key::Generate(4),
                [](Iterable::List<Network::DHT::Node> Result, Network::DHT::EndCallback End)
                {
                    Test::Log("Route") << Result[0].EndPoint << std::endl;
                    End({Network::DHT::Report::Codes::Normal});
                },
                [](const Network::DHT::Report& Report)
                {
                    Test::Log("Route") << "Ended" << std::endl;
                });
        }
        else if (Command == "boot")
        {
            Chord.Bootstrap(
                Target,
                [](const Network::DHT::Report& Report)
                {
                    std::cout << "Bootstrap ended" << std::endl;
                });
        }
        else if (Command == "keys")
        {
            Chord.Keys(
                Target,
                [](const Iterable::List<Cryptography::Key> &keys, Network::DHT::EndCallback End)
                {
                    Test::Log("Keys") << "{ " << keys << " }" << std::endl;
                    End({Network::DHT::Report::Codes::Normal});
                },
                [](const Network::DHT::Report& Report)
                {
                    std::cout << "Keys ended" << std::endl;
                });
        }
        else if (Command == "set")
        {
            std::string Data = "Hello there";
            Chord.Set(
                Cryptography::Key::Generate(4),
                {Data.c_str(), Data.length()},
                [](const Network::DHT::Report& Report)
                {
                    std::cout << "Set ended" << std::endl;
                });
        }
        else if (Command == "get")
        {
            Chord.Get(
                Cryptography::Key::Generate(4),
                [](Iterable::Span<char> &Data, Network::DHT::EndCallback End)
                {
                    Test::Log("Get") << "{ " << Data << " }" << std::endl;
                    End({Network::DHT::Report::Codes::Normal});
                },
                [](const Network::DHT::Report& Report)
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
        else
        {
            Chord.SendTo(Target, {Command.c_str(), Command.length()});
        }

        Command = STDIN.ReadLine();
    }

    Chord.Stop();

    return 0;
}