#include <iostream>
#include <string>

#include <File.hpp>
#include <Cryptography/RSA.hpp>
#include <Cryptography/Digest.hpp>

using namespace Core;

int main(int argc, char const *argv[])
{
    STDOUT.WriteLine("Enter your message");

    std::string Content = STDIN.ReadLine();

    Cryptography::RSA rsa(512, Cryptography::RSA::PKCS1);

    auto Cypher = rsa.Encrypt<Cryptography::RSA::Public>(reinterpret_cast<const unsigned char *>(Content.c_str()), Content.length() + 1);
    auto Plain = rsa.Dencrypt<Cryptography::RSA::Private>(Cypher);

    // auto Cypher = rsa.Encrypt<Cryptography::RSA::Private>(reinterpret_cast<const unsigned char *>(Content.c_str()), Content.length() + 1);
    // auto Plain = rsa.Dencrypt<Cryptography::RSA::Public>(Cypher);

    std::cout << "Plain text is : " << Plain.Content() << std::endl;

    return 0;
}