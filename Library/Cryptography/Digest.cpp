#pragma once

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/md4.h>
#include <openssl/md2.h>

// ## Add SHA3

namespace Core
{
    namespace Cryptography
    {
        template <typename T = SHA_CTX>
        struct HashScheme
        {
            size_t Lenght;
            unsigned char *(*Name)(const unsigned char *, size_t, unsigned char *);
            int (*Init)(T *);
            int (*Update)(T *, const void *, size_t);
            int (*Final)(unsigned char *, T *);
        };

        template <typename T, HashScheme<T> &O>
        class Digest
        {
        private:
            T _State;

        public:
            Digest()
            {
                O.Init(&_State);
            }

            Digest(const Digest &Other) = delete;

            ~Digest() {}

            int Add(unsigned char *Data, size_t Size)
            {
                return O.Update(&_State, Data, Size);
            }

            std::string Hex()
            {
                unsigned char _Content[O.Lenght];
                std::stringstream ss;

                O.Final(_Content, &_State);

                for (int i = 0; i < O.Lenght; i++)
                {
                    ss << std::hex << std::setw(2) << std::setfill('0') << (int)_Content[i];
                }
                return ss.str();
            }

            void Bytes(unsigned char * Data)
            {
                O.Final(Data, &_State);
            }

            Digest &operator=(const Digest &Other) = delete;

            Digest &operator<<(const std::string& Data)
            {
                O.Update(&_State, Data.c_str(), Data.size());
            }

            static size_t Size()
            {
                return O.Lenght;
            }

            static void Bytes(const std::string &Data, unsigned char *Digest)
            {
                O.Name((unsigned char *)Data.c_str(), Data.length(), Digest);
            }

            static void Bytes(const unsigned char *Data, size_t Size, unsigned char *Digest)
            {
                O.Name(Data, Size, Digest);
            }

            // ## Remove this
            static std::string Hex(const std::string &Data)
            {
                unsigned char _Content[O.Lenght];

                O.Name((unsigned char *)Data.c_str(), Data.length(), _Content);

                std::stringstream ss;

                for (size_t i = 0; i < O.Lenght; i++)
                {
                    ss << std::hex << std::setw(2) << std::setfill('0') << (int)_Content[i];
                }

                return ss.str();
            }

            // ## Remove this
            static std::string Hex(const char *Data, size_t Size)
            {
                unsigned char _Content[O.Lenght];

                O.Name((unsigned char *)Data, Size, _Content);

                std::stringstream ss;

                for (size_t i = 0; i < O.Lenght; i++)
                {
                    ss << std::hex << std::setw(2) << std::setfill('0') << (int)_Content[i];
                }

                return ss.str();
            }
        };

        // SHA

#ifndef OPENSSL_NO_SHA1
        HashScheme<SHA_CTX> SHA1Functionality = {
            .Lenght = SHA_DIGEST_LENGTH,
            .Name = ::SHA1,
            .Init = ::SHA1_Init,
            .Update = ::SHA1_Update,
            .Final = ::SHA1_Final,
        };

        typedef Digest<SHA_CTX, SHA1Functionality> SHA1;
#endif

#ifndef OPENSSL_NO_SHA224
        HashScheme<SHA256_CTX> SHA224Functionality = {
            .Lenght = SHA224_DIGEST_LENGTH,
            .Name = ::SHA224,
            .Init = ::SHA224_Init,
            .Update = ::SHA224_Update,
            .Final = ::SHA224_Final,
        };

        typedef Digest<SHA256_CTX, SHA224Functionality> SHA224;
#endif

#ifndef OPENSSL_NO_SHA256
        HashScheme<SHA256_CTX> SHA256Functionality = {
            .Lenght = SHA256_DIGEST_LENGTH,
            .Name = ::SHA256,
            .Init = ::SHA256_Init,
            .Update = ::SHA256_Update,
            .Final = ::SHA256_Final,
        };

        typedef Digest<SHA256_CTX, SHA256Functionality> SHA256;
#endif

#ifndef OPENSSL_NO_SHA384
        HashScheme<SHA512_CTX> SHA384Functionality = {
            .Lenght = SHA384_DIGEST_LENGTH,
            .Name = ::SHA384,
            .Init = ::SHA384_Init,
            .Update = ::SHA384_Update,
            .Final = ::SHA384_Final,
        };

        typedef Digest<SHA512_CTX, SHA384Functionality> SHA384;
#endif

#ifndef OPENSSL_NO_SHA512
        HashScheme<SHA512_CTX> SHA512Functionality = {
            .Lenght = SHA512_DIGEST_LENGTH,
            .Name = ::SHA512,
            .Init = ::SHA512_Init,
            .Update = ::SHA512_Update,
            .Final = ::SHA512_Final,
        };

        typedef Digest<SHA512_CTX, SHA512Functionality> SHA512;
#endif
        // ## MD

#ifndef OPENSSL_NO_MD5
        HashScheme<MD5_CTX> MD5Functionality = {
            .Lenght = MD5_DIGEST_LENGTH,
            .Name = ::MD5,
            .Init = ::MD5_Init,
            .Update = ::MD5_Update,
            .Final = ::MD5_Final,
        };

        typedef Digest<MD5_CTX, MD5Functionality> MD5;
#endif

#ifndef OPENSSL_NO_MD4
        HashScheme<MD4_CTX> MD4Functionality = {
            .Lenght = MD4_DIGEST_LENGTH,
            .Name = ::MD4,
            .Init = ::MD4_Init,
            .Update = ::MD4_Update,
            .Final = ::MD4_Final,
        };

        typedef Digest<MD4_CTX, MD4Functionality> MD4;
#endif
    }
}
