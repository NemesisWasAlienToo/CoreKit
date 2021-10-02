#include <iostream>
#include <string>

#include "Buffer/FIFO.cpp"
#include "Network/Socket.cpp"

using namespace std;

void Assert(const string &Message, bool Result)
{
    if (Result)
        cout << "\033[1;32mPassed\033[0m : " << Message << endl;
    else
        cout << "\033[1;31mFailed\033[0m : " << Message << endl;
}

int main(int argc, char const *argv[])
{
    Core::Network::Socket client(Core::Network::Socket::IPv4, Core::Network::Socket::TCP | Core::Network::Socket::NonBlock);

    Assert("Non blocking", !client.Blocking());

    client.Blocking(true);

    Assert("Blocking", client.Blocking());

    return 0;
}
