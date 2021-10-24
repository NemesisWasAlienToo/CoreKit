#include <iostream>
#include <string>
#include <cstring>
#include <streambuf>
#include <thread>

#include "Base/Test.cpp"
#include "Iterable/List.cpp"
#include "Iterable/Queue.cpp"
#include "Iterable/Poll.cpp"
#include "Network/Socket.cpp"
#include "Network/DNS.cpp"

using namespace Core;
using namespace std;

int main(int argc, char const *argv[])
{
    Iterable::Poll<Network::Socket> Poll(10);

    Iterable::List<Iterable::Queue<char>> Buffers(10);

    Network::EndPoint Host(Network::Address(Network::Address::Any()), 8888);

    Network::Socket server(Network::Socket::IPv4, Network::Socket::TCP);

    server.Bind(Host);

    std::cout << Network::DNS::HostName() << " is listenning on " << Host << std::endl;

    server.Listen(10);

    Poll.Add(server, Descriptor::In);

    Poll.OnRead = [&](const Network::Socket &socket, size_t Index)
    {
        if (socket == server)
        {
            Poll.Add(server.Accept(), Descriptor::In);

            Buffers.Add(Iterable::Queue<char>(1024));

            cout << "Client joined" << endl;
        }
        else
        {

            if (socket.Received() <= 0)
            {
                Poll.Swap(Index);
                Buffers[Index].Free();
                return;
            }

            string Message;

            socket >> Message;

            Buffers.ForEach([&](size_t index, Iterable::Queue<char> &Buffer)
                            {
                                if((Index - 1) != index) {
                                    Buffer.Add(Message.c_str(), Message.length());
                                    Poll(index + 1).events |= Descriptor::Out;
                                } });
        }
    };

    Poll.OnWrite = [&](const Network::Socket &socket, size_t Index)
    {
        if (!Buffers[Index - 1].IsEmpty())
        {
            socket << Buffers[Index - 1];
        }
        else
        {
            Poll(Index).events ^= Descriptor::Out;
        }
    };

    while (1)
    {
        Poll.Await();
    }

    return 0;
}