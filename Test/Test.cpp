#include <iostream>
#include <string>

#include "Network/DNS.cpp"
#include "Base/Converter.cpp"
#include "Cryptography/Digest.cpp"
#include "Cryptography/Random.cpp"
#include "Cryptography/RSA.cpp"

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
    Core::Cryptography::Random::Load();

    std::string Name = Core::Network::DNS::HostName();

    Core::Cryptography::RSA RSA(2048);

    int DSize = RSA.MaxDataSize();

    int CSize = RSA.CypherSize();

    // Hex converter

    unsigned char HexTest[] = {0xE1, 0xB0};

    Assert("Sign verification", Core::Converter::Hex(HexTest, sizeof HexTest) == "e1b0");

    // Enc and Dec

    unsigned char Plain[DSize] = {0};
    unsigned char Cypher[CSize] = {0};

    // ## Encryption

    RSA.Encrypt<Core::Cryptography::RSA::Private>((const unsigned char *)Name.c_str(), Name.length() + 1, Cypher);

    RSA.Dencrypt<Core::Cryptography::RSA::Public>(Cypher, CSize, Plain);

    std::string ENCOut((char *)Plain);

    Assert("Encryption", ENCOut == Name);

    // ## Convertion

    unsigned char CypherPrime[CSize] = {0};

    std::string CypherString = Core::Converter::Hex(Cypher, CSize);

    Core::Converter::Bytes(CypherString, CypherPrime);

    Assert("Convertion", CypherString == Core::Converter::Hex(CypherPrime, CSize));

    // ## Signing

    RSA.Encrypt<Core::Cryptography::RSA::Public>((const unsigned char *)Name.c_str(), Name.length() + 1, Cypher);

    RSA.Dencrypt<Core::Cryptography::RSA::Private>(Cypher, CSize, Plain);

    std::string SIGOut((char *)Plain);

    Assert("Sign", SIGOut == Name);

    // ## Easy Signing

    unsigned char Signature[CSize];

    RSA.Sign<Core::Cryptography::SHA256>("Hello", Signature);

    Assert("Easy Signing", RSA.Verify<Core::Cryptography::SHA256>("Hello", Signature));

    return 0;
}

/*
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <stdio.h>
#include <string.h>

#define KEY_LENGTH 2048
#define PUB_EXP 3
#define PRINT_KEYS
#define WRITE_TO_FILE

int main(void)
{
    size_t pri_len;           // Length of private key
    size_t pub_len;           // Length of public key
    char *pri_key;            // Private key
    char *pub_key;            // Public key
    char msg[KEY_LENGTH / 8]; // Message to encrypt
    char *encrypt = NULL;     // Encrypted message
    char *decrypt = NULL;     // Decrypted message
    char *err;                // Buffer for any error messages

    // Generate key pair
    printf("Generating RSA (%d bits) keypair...", KEY_LENGTH);
    fflush(stdout);
    // RSA *keypair = RSA_generate_key(KEY_LENGTH, PUB_EXP, NULL, NULL);
    RSA *keypair = RSA_new();
    BIGNUM *bn = BN_new();
    BN_zero_ex(bn);
    BN_add_word(bn, 3);

    RSA_generate_key_ex(keypair, KEY_LENGTH, bn, NULL);

    BN_free(bn);

    // To get the C-string PEM form:
    // BIO *pri = BIO_new(BIO_s_mem());
    // BIO *pub = BIO_new(BIO_s_mem());

    // PEM_write_bio_RSAPrivateKey(pri, keypair, NULL, NULL, 0, NULL, NULL);
    // PEM_write_bio_RSAPublicKey(pub, keypair);

    // pri_len = BIO_pending(pri);
    // pub_len = BIO_pending(pub);

    // pri_key = (char *)malloc(pri_len + 1);
    // pub_key = (char *)malloc(pub_len + 1);

    // BIO_read(pri, pri_key, pri_len);
    // BIO_read(pub, pub_key, pub_len);

    // pri_key[pri_len] = '\0';
    // pub_key[pub_len] = '\0';

#ifdef PRINT_KEYS
    // printf("\n%s\n%s\n", pri_key, pub_key);
#endif
    printf("done.\n");

    // Get the message to encrypt
    printf("Message to encrypt: ");
    fgets(msg, KEY_LENGTH - 1, stdin);
    msg[strlen(msg) - 1] = '\0';

    // Encrypt the message
    encrypt = (char *)malloc(RSA_size(keypair));
    int encrypt_len;
    err = (char *)malloc(130);
    if ((encrypt_len = RSA_public_encrypt(strlen(msg) + 1, (unsigned char *)msg, (unsigned char *)encrypt,
                                          keypair, RSA_PKCS1_OAEP_PADDING)) == -1)
    {
        ERR_load_crypto_strings();
        ERR_error_string(ERR_get_error(), err);
        fprintf(stderr, "Error encrypting message: %s\n", err);
        RSA_free(keypair);
        RSA_free(NULL);
        // BIO_free_all(pub);
        // BIO_free_all(pri);
        // free(pri_key);
        // free(pub_key);
        free(encrypt);
        free(decrypt);
        free(err);

        return 0;
    }

#ifdef WRITE_TO_FILE
    // Write the encrypted message to a file
    FILE *out = fopen("out.bin", "w");
    fwrite(encrypt, sizeof(*encrypt), RSA_size(keypair), out);
    fclose(out);
    printf("Encrypted message written to file.\n");
    free(encrypt);
    encrypt = NULL;

    // Read it back
    printf("Reading back encrypted message and attempting decryption...\n");
    encrypt = (char *)malloc(RSA_size(keypair));
    out = fopen("out.bin", "r");
    fread(encrypt, sizeof(*encrypt), RSA_size(keypair), out);
    fclose(out);
#endif

    // Decrypt it
    decrypt = (char *)malloc(encrypt_len);
    if (RSA_private_decrypt(encrypt_len, (unsigned char *)encrypt, (unsigned char *)decrypt,
                            keypair, RSA_PKCS1_OAEP_PADDING) == -1)
    {
        ERR_load_crypto_strings();
        ERR_error_string(ERR_get_error(), err);
        fprintf(stderr, "Error decrypting message: %s\n", err);
        RSA_free(keypair);
        // BIO_free_all(pub);
        // BIO_free_all(pri);
        // free(pri_key);
        // free(pub_key);
        free(encrypt);
        free(decrypt);
        free(err);

        return 0;
    }
    printf("Decrypted message: %s\n", decrypt);

    RSA_free(keypair);
    // BIO_free_all(pub);
    // BIO_free_all(pri);
    // free(pri_key);
    // free(pub_key);
    free(encrypt);
    free(decrypt);
    free(err);

    return 0;
}
*/