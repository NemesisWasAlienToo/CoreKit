#pragma once

#include <iostream>
#include <sstream>
#include <iomanip>

#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/evp.h>

#include "Base/Converter.cpp"

namespace Core
{
    namespace Cryptography
    {
        class RSA
        {
        public:
            // ## Types

            typedef ::RSA Key;

            enum KeyType
            {
                Public,
                Private,
            };

            enum RSAPadding
            {
                None = RSA_NO_PADDING,
                PKCS1 = RSA_PKCS1_PADDING,
                OAEP = RSA_PKCS1_OAEP_PADDING,
                SSL = RSA_SSLV23_PADDING,
            };

        private:
            // ## Variables

            Key *_Keys = NULL;
            RSAPadding _Padding = OAEP;

            static Key * Generate(int Lenght)
            {
                Key *Keys = RSA_new();
                BIGNUM *bn = BN_new();
                BN_set_word(bn, RSA_F4); // What was RSA_F4????

                int Result = RSA_generate_key_ex(Keys, Lenght, bn, NULL);

                BN_free(bn);

                if (Result < 1)
                {
                    unsigned long Error = ERR_get_error();
                    std::cout << "RSA : " << ERR_reason_error_string(Error) << std::endl;
                    exit(-1);
                }

                return Keys;
            }

        public:
            // ## Constructors

            RSA() = default;

            RSA(int Lenght)
            {
                // Check for RAND seed?

                if (_Keys != NULL)
                    RSA_free(_Keys);

                _Keys = Generate(Lenght);
            }

            RSA(Key * Keys)
            {
                _Keys = Keys;
            }

            RSA(RSA& Other) = delete;

            RSA(RSA&& Other){
                std::swap(_Keys, Other._Keys);
            }

            // ## Destructor

            ~RSA()
            {
                RSA_free(_Keys);
            }

            void New()
            {
                if (_Keys != NULL)
                    RSA_free(_Keys);

                _Keys = RSA_new();
            }

            void New(int Length)
            {
                if (_Keys != NULL)
                    RSA_free(_Keys);

                _Keys = Generate(Length);
            }

            // ## Functions

            int DataSize()
            {
                int Extra = 0;
                if (_Padding == PKCS1 || _Padding == SSL)
                    Extra = 11;
                else if (_Padding == OAEP)
                    Extra = 41;

                return RSA_size(_Keys) - Extra;
            }

            int CypherSize()
            {
                return RSA_size(_Keys);
            }

            template <KeyType T>
            void Encrypt(const unsigned char *Plain, int Size, unsigned char *Cypher)
            {

                int Result = 0;

                if (T == Public)
                    Result = RSA_public_encrypt(Size, Plain, Cypher, _Keys, _Padding);
                else if (T == Private)
                    Result = RSA_private_encrypt(Size, Plain, Cypher, _Keys, _Padding == None ? None : PKCS1);

                if (Result < 1)
                {
                    unsigned long Error = ERR_get_error();
                    std::cout << "RSA : " << ERR_reason_error_string(Error) << std::endl;
                    exit(-1);
                }
            }

            template <KeyType T>
            void Dencrypt(const unsigned char *Cypher, int Size, unsigned char *Plain)
            {

                int Result = 0;

                if (T == Public)
                    Result = RSA_public_decrypt(Size, Cypher, Plain, _Keys, _Padding == None ? None : PKCS1);
                else if (T == Private)
                    Result = RSA_private_decrypt(Size, Cypher, Plain, _Keys, _Padding);

                if (Result < 1)
                {
                    unsigned long Error = ERR_get_error();
                    std::cout << "RSA : " << ERR_reason_error_string(Error) << std::endl;
                    exit(-1);
                }
            }

            template <typename T>
            void Sign(const std::string &Data, unsigned char *Signature)
            {
                unsigned char Digest[T::Size()];

                T::Bytes(Data, Digest);

                Encrypt<Private>(Digest, T::Size(), Signature);
            }

            template <typename T>
            bool Verify(const std::string &Data, unsigned char *Signature)
            {
                size_t Size = T::Size();
                unsigned char MDigest[Size];
                unsigned char CDigest[Size];

                T::Bytes(Data, MDigest);

                Dencrypt<Public>(Signature, CypherSize(), CDigest);

                for (size_t i = 0; i < Size; i++)
                    if (MDigest[i] != CDigest[i])
                        return false;

                return true;
            }

            bool Validate()
            {
                return Validate(_Keys);
            }

            // ## Properties

            template <KeyType T>
            std::string Keys()
            {
                std::string str = "";
                if (T == Public)
                {
                    BIO *PublicKeyBuffer = BIO_new(BIO_s_mem());
                    PEM_write_bio_RSAPublicKey(PublicKeyBuffer, _Keys);
                    size_t PublicKeyLen = BIO_pending(PublicKeyBuffer);
                    char *PublicKey = (char *)malloc(PublicKeyLen + 1);
                    BIO_read(PublicKeyBuffer, PublicKey, PublicKeyLen);
                    PublicKey[PublicKeyLen] = '\0';

                    str.append(PublicKey);

                    BIO_free_all(PublicKeyBuffer);
                    free(PublicKey);
                }
                else
                {

                    BIO *PrivateKeyBuffer = BIO_new(BIO_s_mem());
                    PEM_write_bio_RSAPrivateKey(PrivateKeyBuffer, _Keys, NULL, NULL, 0, NULL, NULL);
                    size_t PrivateKeyLen = BIO_pending(PrivateKeyBuffer);
                    char *PrivateKey = (char *)malloc(PrivateKeyLen + 1);
                    BIO_read(PrivateKeyBuffer, PrivateKey, PrivateKeyLen);
                    PrivateKey[PrivateKeyLen] = '\0';

                    str.append(PrivateKey);

                    BIO_free_all(PrivateKeyBuffer);
                    free(PrivateKey);
                }

                return str;
            }

            template <KeyType T>
            void ToFile(const std::string &Name)
            {
                if (T == Public)
                {
                    BIO *PublicKeyBuffer = BIO_new_file(Name.c_str(), "a+");
                    int Result = PEM_write_bio_RSAPublicKey(PublicKeyBuffer, _Keys);

                    if (Result != 1)
                    {
                        unsigned long Error = ERR_get_error();
                        std::cout << "Public Key : " << ERR_reason_error_string(Error) << std::endl;
                    }

                    BIO_free_all(PublicKeyBuffer);
                }
                else
                {

                    BIO *PrivateKeyBuffer = BIO_new_file(Name.c_str(), "a+");
                    int Result = PEM_write_bio_RSAPrivateKey(PrivateKeyBuffer, _Keys, NULL, NULL, 0, NULL, NULL);

                    if (Result != 1)
                    {
                        unsigned long Error = ERR_get_error();
                        std::cout << "Private Key : " << ERR_reason_error_string(Error) << std::endl;
                    }

                    BIO_free_all(PrivateKeyBuffer);
                }
            }

            template <KeyType T>
            static RSA From(const std::string &Value)
            {
                Key * _New = RSA_new();

                if (T == Public)
                {
                    BIO *PublicKeyBuffer = BIO_new(BIO_s_mem());
                    BIO_write(PublicKeyBuffer, Value.c_str(), Value.length());
                    int Result = PEM_write_bio_RSAPublicKey(PublicKeyBuffer, _New);

                    if (Result != 1)
                    {
                        unsigned long Error = ERR_get_error();
                        char Err[200];
                        std::cout << "Public Key : " << ERR_reason_error_string(Error) << std::endl;
                        ERR_error_string(Error, Err);
                        std::cout << "Public Key : " << Err << std::endl;
                    }

                    BIO_free_all(PublicKeyBuffer);
                }
                else
                {

                    BIO *PrivateKeyBuffer = BIO_new(BIO_s_mem());
                    BIO_write(PrivateKeyBuffer, Value.c_str(), Value.length());
                    int Result = PEM_write_bio_RSAPrivateKey(PrivateKeyBuffer, _New, NULL, NULL, 0, NULL, NULL);

                    if (Result != 1)
                    {
                        unsigned long Error = ERR_get_error();
                        std::cout << "Private Key : " << ERR_reason_error_string(Error) << std::endl;
                    }

                    BIO_free_all(PrivateKeyBuffer);
                }

                return _New;
            }

            template <KeyType T>
            static RSA FromFile(const std::string &Name)
            {
                Key * _New = RSA_new();

                if (T == Public)
                {
                    BIO *PublicKeyBuffer = BIO_new_file(Name.c_str(), "r+");
                    Key *Result = PEM_read_bio_RSAPublicKey(PublicKeyBuffer, &_New, NULL, NULL);

                    if (Result != _New)
                    {
                        unsigned long Error = ERR_get_error();
                        std::cout << "Public From : " << ERR_reason_error_string(Error) << std::endl;
                    }

                    BIO_free_all(PublicKeyBuffer);
                }
                else
                {

                    BIO *PrivateKeyBuffer = BIO_new_file(Name.c_str(), "r+");
                    Key * Result = PEM_read_bio_RSAPrivateKey(PrivateKeyBuffer, &_New, NULL, NULL);

                    if (Result != _New)
                    {
                        unsigned long Error = ERR_get_error();
                        std::cout << "Private From : " << ERR_reason_error_string(Error) << std::endl;
                    }

                    BIO_free_all(PrivateKeyBuffer);
                }

                return _New;
            }

            // ##  Static functiosn

            static bool Validate(Key *Keys)
            {
                return RSA_check_key(Keys) == 1;
            }

            // ## Operators

            RSA& operator=(RSA& Other) = delete;

            RSA& operator=(RSA&& Other){
                std::swap(_Keys, Other._Keys);
                return *this;
            }
        };
    }
}