#include <iostream>
#include <string>
#include <cstring>

#include "Iterable/List.cpp"

#include "Network/DNS.cpp"
#include "Network/Socket.cpp"

#include "Base/Poll.cpp"

#define QUEUE 10

int main(int argc, char const *argv[])
{
    Network::Socket server(Network::Socket::IPv4, Network::Socket::TCP);

    Base::Poll<Network::Socket> Watcher(QUEUE + 1);

    auto result = Network::DNS::Host(Network::Address::IPv4);

    Network::EndPoint Host(result[0], 8888);

    server.Bind(Host);

    std::cout << Network::DNS::HostName() <<" is listenning on " << Host << std::endl;

    server.Listen(QUEUE);

    Watcher.Add(server, POLLIN);

    Watcher.OnRead = [&](Network::Socket &socket, int Index)
    {
        if (socket == server)
        {
            Network::EndPoint Info;

            Network::Socket Client = socket.Accept(Info);

            std::cout << Info << " Connected" << std::endl;

            Watcher.Add(Client, Base::Poll<Network::Socket>::In | Base::Poll<Network::Socket>::Out);
        }
        else
        {
            std::string str = "";
            
            if(socket.HasData()){
                std::cout << socket.Peer() << " : " << socket;
                std::cout.flush();
            }
            else{
                std::cout << socket.Peer() << " Disconnected" << str << std::endl;
                Watcher.Remove(socket);
                socket.Close();
            }
        }
    };

    Watcher.OnWrite = [&](Network::Socket &socket, int Index)
    {
        socket << "hello there socket guy\n";
        Watcher(Index).events ^= Base::Poll<Network::Socket>::Out;
    };

    while (1)
    {
        Watcher.Await();
    }
}
