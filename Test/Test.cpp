#include <iostream>
#include <string>

#include "Cryptography/Digest.cpp"

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
    std::cout << Core::Cryptography::SHA1::Hex("Hello ther") << std::endl;
    std::cout << Core::Cryptography::SHA256::Hex("Hello ther") << std::endl;
    std::cout << Core::Cryptography::SHA384::Hex("Hello ther") << std::endl;
    std::cout << Core::Cryptography::SHA512::Hex("Hello ther") << std::endl;
    std::cout << Core::Cryptography::MD5::Hex("Hello ther") << std::endl;
    std::cout << Core::Cryptography::MD4::Hex("Hello ther") << std::endl;

    return 0;
}
