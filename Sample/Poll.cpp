#include <iostream>
#include <string>
#include <functional>

#include "Iterable/List.hpp"
#include "Network/DNS.hpp"
#include "Network/Address.hpp"
#include "Network/Socket.hpp"
#include <Poll.hpp>

using namespace Core;

int main(int argc, char const *argv[])
{
    Core::Poll Poll;
    Iterable::List<Poll::Entry> Events;

    Network::EndPoint Host(Network::Address::Any(), 8888);

    Network::Socket server(Network::Socket::IPv4, Network::Socket::TCP);

    server.Bind(Host);

    server.Listen(10);

    std::cout << Network::DNS::HostName() << " is listenning on " << Host << std::endl;

    Events.Add(server, Poll::In);

    while (1)
    {
        Poll(Events);

        Events.ForEach(
            [&](Poll::Entry &Item)
            {
                if (!Item.HasEvent())
                    return;

                if (Item.Descriptor == server)
                {
                    auto [Client, Info] = server.Accept();

                    Events.Add(Client, Poll::In);

                    std::cout << "New client connected: " << Info << std::endl;
                }
                else
                {
                    auto Data = Item.DescriptorAs<Network::Socket>().Receive();

                    std::cout.write(Data.Content(), Data.Length());
                }
            });
    }

    server.Close();
}