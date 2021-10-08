#include <iostream>
#include <string>

#include "Network/DNS.cpp"
#include "Base/Converter.cpp"
#include "Base/Test.cpp"
#include "Base/File.cpp"
#include "Cryptography/Digest.cpp"
#include "Cryptography/Random.cpp"
#include "Cryptography/RSA.cpp"

using namespace std;

int main(int argc, char const *argv[])
{
    Core::Cryptography::Random::Load();

    std::string Name = Core::Network::DNS::HostName();

    Core::Cryptography::RSA RSA;

    if (Core::File::Exist("Private.pem"))
    {
        RSA = Core::Cryptography::RSA::FromFile<Core::Cryptography::RSA::Private>("Private.pem");
    }
    else
    {
        RSA.New(2048);
        RSA.ToFile<Core::Cryptography::RSA::Private>("Private.pem");
    }

    Core::Test::Assert("RSA Validation", RSA.Validate());

    int DSize = RSA.DataSize();

    int CSize = RSA.CypherSize();

    // Hex converter

    unsigned char HexTest[] = {0xE1, 0xB0};

    Core::Test::Assert("Sign verification", Core::Converter::Hex(HexTest, sizeof HexTest) == "e1b0");

    // Enc and Dec

    unsigned char Plain[DSize];
    unsigned char Cypher[CSize];

    // ## Encryption

    RSA.Encrypt<Core::Cryptography::RSA::Private>((const unsigned char *)Name.c_str(), Name.length() + 1, Cypher);

    RSA.Dencrypt<Core::Cryptography::RSA::Public>(Cypher, CSize, Plain);

    std::string ENCOut((char *)Plain);

    Core::Test::Assert("Encryption", ENCOut == Name);

    // ## Convertion

    unsigned char CypherPrime[CSize];

    std::string CypherString = Core::Converter::Hex(Cypher, CSize);

    Core::Converter::Bytes(CypherString, CypherPrime);

    Core::Test::Assert("Convertion", CypherString == Core::Converter::Hex(CypherPrime, CSize));

    // ## Signing

    RSA.Encrypt<Core::Cryptography::RSA::Public>((const unsigned char *)Name.c_str(), Name.length() + 1, Cypher);

    RSA.Dencrypt<Core::Cryptography::RSA::Private>(Cypher, CSize, Plain);

    std::string SIGOut((char *)Plain);

    Core::Test::Assert("Sign", SIGOut == Name);

    // ## Easy Signing

    unsigned char Signature[CSize];

    RSA.Sign<Core::Cryptography::SHA256>("Hello", Signature);

    Core::Test::Assert("Easy Signing", RSA.Verify<Core::Cryptography::SHA256>("Hello", Signature));

    return 0;
}