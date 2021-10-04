#include <iostream>
#include <string>

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
    std::cout << Core::Cryptography::SHA256::Hex(Core::Cryptography::Random::Hex(2)) << std::endl;

    return 0;
}
