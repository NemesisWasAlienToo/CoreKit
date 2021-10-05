#include <iostream>
#include <string>

#include "Network/DNS.cpp"
#include "Cryptography/Digest.cpp"
#include "Cryptography/Random.cpp"

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
    Core::Cryptography::Random::Load(Core::Cryptography::Random::random, 32);

    std::string Name = Core::Network::DNS::HostName();
    std::string Nonce = Core::Cryptography::Random::Hex(16);
    std::string Hash = Core::Cryptography::SHA256::Hex(Name + Nonce);

    while (Hash.substr(0, 2) != "00")
    {
        Nonce = Core::Cryptography::Random::Hex(16);
        Hash = Core::Cryptography::SHA256::Hex(Name + Nonce);
        usleep(10000);
    }

    std::cout << "Nonce is : " << Nonce << std::endl;

    return 0;
}
