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

    Cryptography::AES<Cryptography::AESModes::ECB, 128> AES(Cryptography::Key::Generate(128 / 8), Cryptography::Key::Generate(128 / 8));

    auto Cypher = AES.Encrypt(reinterpret_cast<const unsigned char *>(Content.c_str()), Content.length() + 1);

    auto Plain = AES.Decrypt(Cypher);

    std::cout << "Plain: " << Plain.Content() << std::endl;

    return 0;
}