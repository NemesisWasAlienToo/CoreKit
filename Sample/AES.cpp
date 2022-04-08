#include <iostream>
#include <string>

#include <File.hpp>
#include <Cryptography/Key.hpp>
#include <Cryptography/AES.hpp>

using namespace Core;

int main(int argc, char const *argv[])
{
    STDOUT.WriteLine("Enter your message");

    std::string Content = STDIN.ReadLine();

    Cryptography::AES<256> AES(Cryptography::Key::Generate(256 / 8), Cryptography::Key::Generate(128 / 8));

    auto Cypher = AES.Encrypt<Cryptography::AESModes::CTR>(reinterpret_cast<const unsigned char *>(Content.c_str()), Content.length() + 1);

    auto Plain = AES.Decrypt<Cryptography::AESModes::CTR>(Cypher);

    std::cout << "Plain: " << Plain.Content() << std::endl;

    return 0;
}