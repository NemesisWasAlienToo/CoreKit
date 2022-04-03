#include <iostream>
#include <string>
#include <unistd.h>

#include <File.hpp>
#include <Coroutine.hpp>
#include <Network/DHT/DHT.hpp>
#include <Network/DHT/Chord.hpp>
#include <Network/DHT/Runner.hpp>

using namespace Core;

int main(int argc, char const *argv[])
{
    const Network::EndPoint Target{"127.0.0.1:4444"};

    Network::DHT::Node Identity(Cryptography::Key::Generate(4), {"0.0.0.0:8888"});

    Network::DHT::Runner<Network::DHT::Chord> Node(Identity, {5, 0});

    Node.OnData = [](const Network::DHT::Node &Node, Format::Serializer &Serializer)
    {
        std::cout << Node.Id << " : ";

        Serializer.Take<Iterable::Span<char>>().ForEach(
            [](char c)
            {
                std::cout << c;
            });

        std::cout << std::endl;
    };

    Node.Run(1);

    // Perform test

    while (true)
    {
        STDOUT.WriteLine("Waiting for commands");

        std::string Command = STDIN.ReadLine();

        if (Command == "ping")
        {
            Node.Ping(
                Target,
                [](Duration duration, auto End)
                {
                    Test::Log("Ping") << duration << std::endl;
                    End();
                },
                []
                {
                    Test::Log("Ping") << "Ended\n";
                });
        }

        else if (Command == "query")
        {
            Node.Query(
                Target,
                Cryptography::Key::Generate(4),
                [](Iterable::List<Network::DHT::Node> Result, auto End)
                {
                    Test::Log("Query") << Result[0].EndPoint << std::endl;
                    End();
                },
                []
                {
                    Test::Log("Query") << "Ended" << std::endl;
                });
        }

        // Needs to re-written

        else if (Command == "route")
        {
            Node.Route(
                Cryptography::Key::Generate(4),
                [](Iterable::List<Network::DHT::Node> Result, auto End)
                {
                    Test::Log("Route") << Result[0].EndPoint << std::endl;
                    End();
                },
                []
                {
                    Test::Log("Route") << "Ended" << std::endl;
                });
        }
        else if (Command == "boot")
        {
            Node.Bootstrap(
                Target,
                []
                {
                    Test::Log("Bootstrap") << "Ended" << std::endl;
                });
        }
        else if (Command == "test")
        {
            int count = 100, i;
            Duration total;

            i = count;

            while (i--)
            {
                Node.Ping(
                    Target,
                    [&total](Duration duration, auto End)
                    {
                        std::cout << duration << std::endl;

                        total.Seconds += duration.Seconds;
                        total.Nanoseconds += duration.Nanoseconds;
                    },
                    nullptr);

                usleep(50000);
            }

            total.Seconds /= count;
            total.Nanoseconds /= count;

            Test::Log("Test") << total << std::endl;
        }
        else if (Command == "exit")
        {
            break;
        }
        else
        {
            Node.ForEach(
                [&](const auto& Nodes)
                {
                    Node.Data(Nodes[0].EndPoint, Command);
                }
            );
        }
    }

    Node.Stop();

    return 0;
}