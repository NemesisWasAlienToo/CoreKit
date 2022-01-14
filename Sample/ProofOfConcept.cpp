#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <functional>

#include <Test.cpp>
#include <Timer.cpp>
#include <Iterable/List.cpp>
#include <Iterable/Poll.cpp>
#include <Network/DNS.cpp>
#include <Network/Socket.cpp>
#include <Network/EndPoint.cpp>

using namespace Core;

Network::Socket Socket(Network::Socket::IPv4, Network::Socket::UDP);

Timer timer(Timer::Monotonic, 0);

Iterable::Poll Poll(2);

std::mutex Lock;

void Loop()
{
    Network::EndPoint Target;

    while (true)
    {
        Lock.lock();

        Poll();

        std::cout << "Received" << std::endl;

        if (Poll[0].HasEvent())
        {
            size_t len = Socket.Received();

            char Data[len];

            Socket.ReceiveFrom(Data, len, Target);

            Lock.unlock();

            Data[len] = 0;

            std::cout << Data << std::endl;
        }
        else if (Poll[1].HasEvent())
        {
            timer.Set({3, 0});

            Lock.unlock();
        }
    }
}

int main(int argc, char const *argv[])
{
    Socket.Bind({"0.0.0.0:8888"});

    Poll.Add(Socket, Iterable::Poll::In);
    Poll.Add(timer, Iterable::Poll::In);

    timer.Set({3, 0});

    Iterable::List<std::thread> Pool(5);

    Pool.Reserve(Pool.Capacity());

    Pool.ForEach(
        [](std::thread &Thread)
        {
            Thread = std::thread(Loop);
        });

    Loop();

    Pool.ForEach(
        [](std::thread &Thread)
        {
            Thread.join();
        });

    return 0;
}