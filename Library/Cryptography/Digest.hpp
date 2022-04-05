#pragma once

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/md4.h>

#include <Iterable/Span.hpp>

// ## Add SHA3

namespace Core
{
    namespace Cryptography
    {
        template <typename TState>
        struct Functionality
        {
            typedef TState StateType;
            typedef unsigned char *(*TName)(const unsigned char *, size_t, unsigned char *);
            typedef int (*TInit)(TState *);
            typedef int (*TUpdate)(TState *, const void *, size_t);
            typedef int (*TFinal)(unsigned char *, TState *);
        };

        template <typename Scheme>
        class Digest
        {
        private:
            Scheme _State;

        public:
            Digest()
            {
                Scheme::Init(&_State);
            }

            Digest(const Digest &Other) = delete;

            ~Digest() = default;

            int Add(unsigned char *Data, size_t Size)
            {
                return Scheme::Update(&_State, Data, Size);
            }

            int Add(const Iterable::Span<char> Data)
            {
                return Scheme::Update(&_State, Data.Content(), Data.Length());
            }

            void Bytes(unsigned char *Data)
            {
                Scheme::Final(Data, &_State);
            }

            Iterable::Span<char> Bytes()
            {
                Iterable::Span<char> Res(Scheme::Lenght);

                Scheme::Final(Res.Content(), &_State);

                return Res;
            }

            // Static funtions

            inline static constexpr size_t Size()
            {
                return Scheme::Lenght;
            }

            static void Bytes(const unsigned char *Data, size_t Size, unsigned char *Digest)
            {
                Scheme::Name(Data, Size, Digest);
            }

            static Iterable::Span<char> Bytes(const unsigned char *Data, size_t Size)
            {
                Iterable::Span<char> Res(Scheme::Lenght);

                Scheme::Name(Data, Size, reinterpret_cast<unsigned char *>(Res.Content()));

                return Res;
            }

            static Iterable::Span<char> Bytes(const Iterable::Span<char> &Data)
            {
                Iterable::Span<char> Res(Scheme::Lenght);

                Scheme::Name(reinterpret_cast<const unsigned char *>(Data.Content()), Data.Length(), Res);

                return Res;
            }

            // Operators

            Digest &operator=(const Digest &Other) = delete;
        };

        // SHA

#ifndef OPENSSL_NO_SHA1
        struct SHA1Functionality : Functionality<SHA_CTX>
        {
            static constexpr size_t Lenght = SHA_DIGEST_LENGTH;
            static constexpr TName Name = ::SHA1;
            static constexpr TInit Init = ::SHA1_Init;
            static constexpr TUpdate Update = ::SHA1_Update;
            static constexpr TFinal Final = ::SHA1_Final;
        };

        typedef Digest<SHA1Functionality> SHA1;
#endif

#ifndef OPENSSL_NO_SHA224
        struct SHA224Functionality : Functionality<SHA256_CTX>
        {
            static constexpr size_t Lenght = SHA224_DIGEST_LENGTH;
            static constexpr TName Name = ::SHA224;
            static constexpr TInit Init = ::SHA224_Init;
            static constexpr TUpdate Update = ::SHA224_Update;
            static constexpr TFinal Final = ::SHA224_Final;
        };

        typedef Digest<SHA224Functionality> SHA224;
#endif

#ifndef OPENSSL_NO_SHA256
        struct SHA256Functionality : Functionality<SHA256_CTX>
        {
            static constexpr size_t Lenght = SHA256_DIGEST_LENGTH;
            static constexpr TName Name = ::SHA256;
            static constexpr TInit Init = ::SHA256_Init;
            static constexpr TUpdate Update = ::SHA256_Update;
            static constexpr TFinal Final = ::SHA256_Final;
        };

        typedef Digest<SHA256Functionality> SHA256;
#endif

#ifndef OPENSSL_NO_SHA384
        struct SHA384Functionality : Functionality<SHA512_CTX>
        {
            static constexpr size_t Lenght = SHA384_DIGEST_LENGTH;
            static constexpr TName Name = ::SHA384;
            static constexpr TInit Init = ::SHA384_Init;
            static constexpr TUpdate Update = ::SHA384_Update;
            static constexpr TFinal Final = ::SHA384_Final;
        };

        typedef Digest<SHA384Functionality> SHA384;
#endif

#ifndef OPENSSL_NO_SHA512
        struct SHA512Functionality : Functionality<SHA512_CTX>
        {
            static constexpr size_t Lenght = SHA512_DIGEST_LENGTH;
            static constexpr TName Name = ::SHA512;
            static constexpr TInit Init = ::SHA512_Init;
            static constexpr TUpdate Update = ::SHA512_Update;
            static constexpr TFinal Final = ::SHA512_Final;
        };

        typedef Digest<SHA512Functionality> SHA512;
#endif
        // ## MD

#ifndef OPENSSL_NO_MD5
        struct MD5Functionality : Functionality<MD5_CTX>
        {
            static constexpr size_t Lenght = MD5_DIGEST_LENGTH;
            static constexpr TName Name = ::MD5;
            static constexpr TInit Init = ::MD5_Init;
            static constexpr TUpdate Update = ::MD5_Update;
            static constexpr TFinal Final = ::MD5_Final;
        };

        typedef Digest<MD5Functionality> MD5;
#endif

#ifndef OPENSSL_NO_MD4
        struct MD4Functionality : Functionality<MD4_CTX>
        {
            static constexpr size_t Lenght = MD4_DIGEST_LENGTH;
            static constexpr TName Name = ::MD4;
            static constexpr TInit Init = ::MD4_Init;
            static constexpr TUpdate Update = ::MD4_Update;
            static constexpr TFinal Final = ::MD4_Final;
        };

        typedef Digest<MD4Functionality> MD4;
#endif
    }
}
