#include <iostream>
#include <string>
#include <cstring>
#include <streambuf>
#include "Iterable/List.cpp"
#include "Iterable/Buffer.cpp"
#include "Network/DNS.cpp"
#include "Network/Socket.cpp"
#include "Network/HTTP.cpp"
#include "Cryptography/Digest.cpp"
#include "Cryptography/Random.cpp"
#include "Base/Test.cpp"
#include "Iterable/Buffer.cpp"

using namespace Core;
using namespace std;

int main(int argc, char const *argv[])
{
    Iterable::Buffer<char> Buffer(10);

    Buffer.Add('b', 5);

    cout << Buffer.Peek() << endl;

    Buffer.Add('a', 5);

    cout << Buffer.Peek() << endl;

    Buffer.Remove(1);

    Buffer.Free(2);

    Buffer.Add('c', 3);

    cout << Buffer.Peek() << endl;

    return 0;
}