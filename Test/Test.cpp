#include <iostream>
#include <string>
#include <cstring>
#include <streambuf>
#include <thread>

#include "Base/Test.cpp"
#include "Iterable/List.cpp"
#include "Iterable/Queue.cpp"
#include "Iterable/Iterable.cpp"
#include "Iterable/Poll.cpp"
#include "Network/Socket.cpp"
#include "Network/DNS.cpp"

using namespace Core;
using namespace std;

int main(int argc, char const *argv[])
{
    bool Running = true;

    Iterable::Poll Poll(10);

    Iterable::List<Iterable::Queue<char>> Buffers(10);

    // Launch server

    Network::EndPoint Host(Network::Address::Any(Network::Address::IPv4), 8888);

    Network::Socket server(Network::Socket::IPv4, Network::Socket::TCP);

    server.Bind(Host);

    std::cout << Network::DNS::HostName() << " is listenning on " << Host << std::endl;

    server.Listen(10);

    Poll.Add(server, Iterable::Poll::In);

    // Connect to another peer

    Network::Socket client(Network::Socket::IPv4, Network::Socket::TCP | Network::Socket::NonBlocking);

    client.Connect(Network::Address("37.152.181.245"), 8888);

    Poll.Add(client, Iterable::Poll::In);

    Buffers.Add(Iterable::Queue<char>(1024));

    Poll.OnRead = [&](Network::Socket socket, size_t Index)
    {
        if (socket == server)
        {
            Poll.Add(server.Accept(), Iterable::Poll::In);

            Buffers.Add(Iterable::Queue<char>(1024));

            cout << "Client joined" << endl;
        }
        else
        {
            // if (!socket.IsConnected())
            if (socket.Received() <= 0)
            {
                socket.Close();
                Poll.Swap(Index);
                Buffers.Swap(Index - 1);
                return;
            }

            string Message;
            socket >> Message;

            if(Message.find("END") != string::npos) Running = false;

            Buffers.ForEach([&](size_t index, Iterable::Queue<char> &Buffer)
                            {
                if(index != Index - 1){
                    Buffer << Message;
                    Poll(index + 1).events |= Iterable::Poll::Out;
                } });
        }
    };

    Poll.OnWrite = [&](Network::Socket socket, size_t Index)
    {
        if (!Buffers[Index - 1].IsEmpty())
        {
            socket << Buffers[Index - 1];
        }
        else
        {
            Poll(Index).events ^= Iterable::Poll::Out;
        }
    };

    while (Running)
    {
        Poll.Await();
    }

    return 0;
}