// # Standard headers

#include <iostream>
#include <string>
#include <mutex>

// # User headers

#include <Test.cpp>
#include <DateTime.cpp>
#include <Iterable/List.cpp>

#include <Format/Hex.cpp>
#include <Format/Serializer.cpp> // <----- @todo Strange error

#include <Network/DNS.cpp>
#include <Network/DHT/Server.cpp>
#include <Network/DHT/Handler.cpp>
#include <Network/DHT/Chord.cpp>

// # Usings

using namespace Core;

// # Global variables

int main(int argc, char const *argv[])
{
    unsigned short Port = 4444;

    Network::DHT::Key Identity = Network::DHT::Key::Generate(32);

    if(argc == 2)
    {
        Port = std::atol(argv[1]);
    }

    Network::EndPoint EndPoint(Network::Address::Any(Network::Address::IPv4), Port);

    Network::EndPoint Server("127.0.0.1:8888");

    Network::Socket Socket(Network::Socket::IPv4, Network::Socket::UDP);

    std::string Command;
    std::string Message;

    Socket.Bind(EndPoint);

    std::cout << Network::DNS::HostName() << " is listenning with udp on " << EndPoint << std::endl;

    while (true)
    {
        std::cout << "Command : ";

        std::cin >> Command;

        if (Command == "exit")
        {
            break;
        }
        else if (Command == "ping")
        {
            Iterable::Queue<char> q;
            Format::Serializer s(q);

            // Maybe add should give an index to be used to modify later?

            s.Add((char *) "CHRD", 4) << (int) 0 << (char) Network::DHT::Chord::Runner::Operations::Ping << Identity;

            s.Modify<uint32_t>(4) = htonl(q.Length());

            Socket.SendTo(q.Content(), q.Length(), Server);

            Socket.Await(Network::Socket::In);

            size_t len = Socket.Received();

            char Buffer[len + 1];

            Network::EndPoint Target;

            Socket.ReceiveFrom(Buffer, len, Target);

            std::cout << Target << " Said (" << len - 9 << ") " << Format::Hex::From(&Buffer[9], len - 9) << std::endl;
        }
        else if (Command == "send")
        {
            std::cout << "Message : ";

            std::cin >> Message;

            char Signiture[8] = {'C', 'H', 'R', 'D', 0, 0, 0, 0};

            auto len = Message.length() + 1;

            *(uint32_t *)(&Signiture[4]) = htonl(len + sizeof(Signiture));

            Socket.SendTo(Signiture, sizeof(Signiture), Server);

            Socket.SendTo(Message.c_str(), len, Server);
        }
        else if (Command == "listen")
        {
            Socket.Await(Network::Socket::In);

            size_t len = Socket.Received();
            char Buffer[len + 1];
            Network::EndPoint Target;

            Socket.ReceiveFrom(Buffer, len, Target);

            Buffer[len] = 0;

            std::cout << Target << " Said (" << len - 9 << ") " << Format::Hex::From(&Buffer[9], len - 9) << std::endl;

            char Signiture[8] = {'C', 'H', 'R', 'D', 0, 0, 0, 0};

            std::cout << "Message : ";

            std::cin >> Message;

            len = Message.length() + 1;

            *(uint32_t *)(&Signiture[4]) = htonl(len + sizeof(Signiture));

            Socket.SendTo(Signiture, sizeof(Signiture), Target);

            Socket.SendTo(Message.c_str(), len, Target);
        }

        Command = "";
    }

    Socket.Close();

    return 0;
}