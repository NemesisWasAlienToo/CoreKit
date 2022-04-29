#include <iostream>
#include <string>
#include <functional>

#include "Iterable/List.hpp"
#include "Network/DNS.hpp"
#include "Network/Address.hpp"
#include "Network/Socket.hpp"
#include <ePoll.hpp>

using namespace Core;

int main(int argc, char const *argv[])
{
    Core::ePoll Poll;

    Network::EndPoint Host(Network::Address::Any(), 8888);

    Network::Socket server(Network::Socket::IPv4, Network::Socket::TCP);

    server.Bind(Host);

    std::cout << Network::DNS::HostName() << " is listenning on " << Host << std::endl;

    server.Listen(10);

    Poll.Add(server, ePoll::In);

    Iterable::List<ePoll::Container> Events(10);

    while (1)
    {
        Poll(Events);

        Events.ForEach(
            [&](ePoll::Container &Item)
            {
                auto Socket = Item.DataAs<Network::Socket>();

                if (Socket == server)
                {
                    auto [Client, Info] = Socket.Accept();

                    Poll.Add(Client, ePoll::In);

                    std::cout << "New client connected: " << Info << std::endl;
                }
                else
                {
                    if (Item.Happened(ePoll::In))
                    {
                        // check if client has disconnected

                        if (Socket.Received() == 0)
                        {
                            Poll.Delete(Socket);

                            Socket.Close();

                            std::cout << "Client disconnected" << std::endl;

                            return;
                        }

                        auto Data = Item.DataAs<Network::Socket>().Receive();

                        std::cout.write(Data.Content(), Data.Length());
                    }
                }
            });
    }

    server.Close();
}