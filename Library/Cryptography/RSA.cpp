#pragma once

#include <iostream>
#include <sstream>
#include <iomanip>

#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/err.h>

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
                Any,
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

        public:
            // ## Constructors

            RSA() = default;

            RSA(int Lenght, int Exponent = 3)
            {
                // Check for RAND seed?
                Generate(Lenght, Exponent);
            }

            // ## Destructor

            ~RSA()
            {
                RSA_free(_Keys);
            }

            // ## Functions

            int MaxDataSize()
            {
                int Extra = 0;
                if (_Padding == PKCS1 || _Padding == SSL)
                    Extra = 11;
                else if (_Padding == OAEP)
                    Extra = 41;

                return RSA_size(_Keys) - Extra;
            }

            int MinDataSize()
            {
                return _Padding == None ? MaxDataSize() : 0;
            }

            int CypherSize()
            {
                return RSA_size(_Keys);
            }

            void Generate(int Lenght, int Exponent = 3)
            {
                if (_Keys != NULL)
                    RSA_free(_Keys);

                _Keys = RSA_new();
                BIGNUM *bn = BN_new();
                BN_zero_ex(bn);
                BN_add_word(bn, Exponent);

                int Result = RSA_generate_key_ex(_Keys, Lenght, bn, NULL);

                BN_free(bn);

                if (Result < 1)
                {
                    unsigned long Error = ERR_get_error();
                    std::cout << "RSA : " << ERR_reason_error_string(Error) << std::endl;
                    exit(-1);
                }
            }

            void From(Key *keys)
            {
                _Keys = keys;
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

            // ## Properties

            Key *Keys() { return _Keys; }

            // ##  Static functiosn

            // static Key * Generate(int Lenght, int Exponent = 3)
            // {
            //     auto tmp = RSA_new();
            //     BIGNUM *bn = BN_new();
            //     BN_zero_ex(bn);
            //     BN_add_word(bn, Exponent);

            //     int Result = RSA_generate_key_ex(_Keys, Lenght, bn, NULL);

            //     BN_free(bn);

            //     if (Result < 1)
            //     {
            //         unsigned long Error = ERR_get_error();
            //         std::cout << "RSA : " << ERR_reason_error_string(Error) << std::endl;
            //         exit(-1);
            //     }

            //     return tmp;
            // }

            // ## Operators
        };
    }
}