#include <iostream>
#include <string>
#include <functional>

#include "Iterable/List.hpp"
#include "Network/DNS.hpp"
#include "Network/Address.hpp"
#include "Network/Socket.hpp"
#include <Iterable/ePoll.hpp>

using namespace Core;

int main(int argc, char const *argv[])
{
    Iterable::ePoll Poll;

    Core::Network::EndPoint Host(Core::Network::Address(Core::Network::Address::Any()), 8888);

    Core::Network::Socket server(Core::Network::Socket::IPv4, Core::Network::Socket::TCP);

    server.Bind(Host);

    std::cout << Core::Network::DNS::HostName() << " is listenning on " << Host << std::endl;

    server.Listen(10);

    Poll.Add(server, Iterable::ePoll::In);

    Iterable::List<Iterable::ePoll::Container> Events(10);

    while (1)
    {
        Poll(Events);

        Events.ForEach(
            [&](Iterable::ePoll::Container &Item)
            {
                if (Item.DataAs<int>() == server.INode())
                {
                    auto [Client, Info] = server.Accept();

                    Poll.Add(Client, EPOLLIN);

                    std::cout << "New client connected: " << Info << std::endl;
                }
                else
                {
                    if (Item.Happened(EPOLLIN))
                    {
                        auto Data = Item.DataAs<Core::Network::Socket>().Receive();

                        std::cout.write(Data.Content(), Data.Length());
                    }
                }
            });
    }

    server.Close();
}