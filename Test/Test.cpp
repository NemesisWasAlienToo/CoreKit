#include <iostream>
#include <string>
#include <functional>

#include <Test.cpp>
#include <DateTime.cpp>
#include <Iterable/List.cpp>
#include <Iterable/Queue.cpp>
#include <Network/DNS.cpp>
#include <Network/DHT/Server.cpp>
#include <Network/DHT/Handler.cpp>
#include <Network/DHT/Chord.cpp>

using namespace Core;

int main(int argc, char const *argv[])
{
    Iterable::Queue<char> q;
    Format::Serializer s(q);

    q.Add("hello there", 12);

    char b[12] = {0};

    q.Take(b, 12);

    std::cout << b << std::endl;

    char c[12] = {0};

    q.Take(c, 12);

    std::cout << c << std::endl;

    return 0;
}