#include <iostream>
#include <string>

#include <Cryptography/Digest.hpp>
#include <Format/Hex.hpp>

using namespace Core;

int main(int argc, char const *argv[])
{
    std::string Content;

    std::cout << "Enter your text" << std::endl;
    std::cin >> Content;

    auto Hash = Cryptography::SHA256::Bytes(reinterpret_cast<const unsigned char*>(Content.c_str()), Content.length());

    std::cout << Format::Hex::From(Hash) << std::endl;

    return 0;
}
