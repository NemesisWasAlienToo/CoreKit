#pragma once

#include <iostream>
#include <sstream>
#include <iomanip>

#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/engine.h>

#include <Format/Hex.hpp>
#include <Format/Base64.hpp>
#include <Iterable/Span.hpp>

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

        public:
            // ## Constructors

            RSA() = default;

            RSA(int BitLenght, RSAPadding Padding = OAEP) : _Keys(Generate(BitLenght)), _Padding(Padding) {}

            RSA(Key *Keys)
            {
                _Keys = Keys;
            }

            RSA(const RSA &Other) = delete;

            RSA(RSA &&Other)
            noexcept : _Keys(Other._Keys), _Padding(Other._Padding)
            {
                Other._Keys = nullptr;
                Other._Padding = OAEP;
            }

            // ## Destructor

            ~RSA()
            {
                RSA_free(_Keys);
            }

            // ## Functions

            int PlainSize()
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
            int Encrypt(const unsigned char *Plain, int Size, unsigned char *Cypher)
            {

                int Result = 0;

                if constexpr (T == Public)
                    Result = RSA_public_encrypt(Size, Plain, Cypher, _Keys, _Padding);
                else if constexpr (T == Private)
                    Result = RSA_private_encrypt(Size, Plain, Cypher, _Keys, _Padding == None ? None : PKCS1);

                if (Result < 1)
                {
                    unsigned long Error = ERR_get_error();
                    std::cout << "RSA : " << ERR_reason_error_string(Error) << std::endl;
                    exit(-1);
                }

                return Result;
            }

            template <KeyType T>
            Iterable::Span<char> Encrypt(const unsigned char *Plain, int Size)
            {
                int Result = 0;
                Iterable::Span<char> Cypher(CypherSize());

                if constexpr (T == Public)
                    Result = RSA_public_encrypt(Size, Plain, reinterpret_cast<unsigned char *>(Cypher.Content()), _Keys, _Padding);
                else if constexpr (T == Private)
                    Result = RSA_private_encrypt(Size, Plain, reinterpret_cast<unsigned char *>(Cypher.Content()), _Keys, _Padding == None ? None : PKCS1);

                if (Result < 1)
                {
                    unsigned long Error = ERR_get_error();
                    std::cout << "RSA : " << ERR_reason_error_string(Error) << std::endl;
                    exit(-1);
                }

                // @todo Optimize this later
                
                if(Result != CypherSize())
                    Cypher.Resize(Result);

                return Cypher;
            }

            template <KeyType T>
            Iterable::Span<char> Encrypt(const Iterable::Span<char> &Plain)
            {
                return Encrypt<T>(reinterpret_cast<const unsigned char *>(Plain.Content()), Plain.Length());
            }

            template <KeyType T>
            int Dencrypt(const unsigned char *Cypher, int Size, unsigned char *Plain)
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

                return Result;
            }

            template <KeyType T>
            Iterable::Span<char> Dencrypt(const unsigned char *Cypher, int Size)
            {
                int Result = 0;
                Iterable::Span<char> Plain(PlainSize());

                if (T == Public)
                    Result = RSA_public_decrypt(Size, Cypher, reinterpret_cast<unsigned char *>(Plain.Content()), _Keys, _Padding == None ? None : PKCS1);
                else if (T == Private)
                    Result = RSA_private_decrypt(Size, Cypher, reinterpret_cast<unsigned char *>(Plain.Content()), _Keys, _Padding);

                if (Result < 1)
                {
                    unsigned long Error = ERR_get_error();
                    std::cout << "RSA : " << ERR_reason_error_string(Error) << std::endl;
                    exit(-1);
                }

                // @todo Optimize this later

                if(Result != PlainSize())
                    Plain.Resize(Result);

                return Plain;
            }

            template <KeyType T>
            Iterable::Span<char> Dencrypt(const Iterable::Span<char> &Cypher)
            {
                return Dencrypt<T>(reinterpret_cast<const unsigned char *>(Cypher.Content()), Cypher.Length());
            }

            template <typename T>
            void Sign(const unsigned char *Data, size_t Size, unsigned char *Signature)
            {
                unsigned char Digest[T::Size()];

                T::Bytes(Data, Size, Digest);

                Encrypt<Private>(Digest, T::Size(), Signature);
            }

            template <typename T>
            Iterable::Span<char> Sign(const unsigned char *Data, size_t Size)
            {
                unsigned char Digest[T::Size()];

                T::Bytes(Data, Size, Digest);

                return Encrypt<Private>(Digest, T::Size());
            }

            template <typename T>
            Iterable::Span<char> Sign(const Iterable::Span<char> &Data)
            {
                unsigned char Digest[T::Size()];

                T::Bytes(Data.Content(), Data.Length(), Digest);

                return Encrypt<Private>(Digest, T::Size());
            }

            // @todo Implenet
            // template <typename T>
            // void PKCSSign(const std::string &Data, unsigned char *Signature)
            // {
            // }

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

            // @todo Implenet
            // template <typename T>
            // bool PKCSVerify(const std::string &Data, unsigned char *Signature)
            // {
            // }

            bool IsValid()
            {
                return IsValid(_Keys);
            }

            // ## Properties

            Key *Keys() const
            {
                return _Keys;
            }

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

            // @todo Implement

            // template <KeyType T>
            // void Keys(Key *Keys)
            // {
            //     if constexpr (T == Public)
            //     {
            //     }
            //     else
            //     {
            //     }
            // }

            template <KeyType T>
            void ToFile(const std::string &Name)
            {
                BIO *KeyBuffer = BIO_new_file(Name.c_str(), "a+");
                int Result = 0;

                if (T == Public)
                {
                    Result = PEM_write_bio_RSAPublicKey(KeyBuffer, _Keys);
                }
                else
                {
                    Result = PEM_write_bio_RSAPrivateKey(KeyBuffer, _Keys, NULL, NULL, 0, NULL, NULL);
                }

                if (Result != 1)
                {
                    unsigned long Error = ERR_get_error();
                    std::cout << "Key : " << ERR_reason_error_string(Error) << std::endl;
                }

                BIO_free_all(KeyBuffer);
            }

            // ##  Static functiosn

            template <KeyType T>
            static RSA From(const std::string &Value)
            {
                Key *_New = RSA_new();

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
                Key *_New = RSA_new();

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
                    Key *Result = PEM_read_bio_RSAPrivateKey(PrivateKeyBuffer, &_New, NULL, NULL);

                    if (Result != _New)
                    {
                        unsigned long Error = ERR_get_error();
                        std::cout << "Private From : " << ERR_reason_error_string(Error) << std::endl;
                    }

                    BIO_free_all(PrivateKeyBuffer);
                }

                return _New;
            }

            static inline bool IsValid(Key *Keys)
            {
                return RSA_check_key(Keys) == 1;
            }

            static inline Key *Generate()
            {
                return RSA_new();
            }

            static Key *Generate(int BitLenght)
            {
                Key *Keys = RSA_new();
                BIGNUM *bn = BN_new();
                BN_set_word(bn, RSA_F4);

                int Result = RSA_generate_key_ex(Keys, BitLenght, bn, NULL);

                BN_free(bn);

                if (Result < 1)
                {
                    unsigned long Error = ERR_get_error();
                    std::cout << "RSA : " << ERR_reason_error_string(Error) << std::endl;
                    exit(-1);
                }

                return Keys;
            }

            // ## Operators

            RSA &operator=(const RSA &Other) = delete;

            RSA &operator=(RSA &&Other) noexcept
            {
                if (this == &Other)
                {
                    return *this;
                }

                RSA_free(_Keys);

                _Keys = Other._Keys;
                _Padding = Other._Padding;

                Other._Keys = nullptr;
                Other._Padding = OAEP;

                return *this;
            }
        };
    }
}