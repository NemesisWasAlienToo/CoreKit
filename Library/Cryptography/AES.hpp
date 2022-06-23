#pragma once

#include <iostream>
#include <sstream>
#include <iomanip>

#include <openssl/bn.h>
#include <openssl/aes.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/engine.h>

#include <Cryptography/Key.hpp>

namespace Core::Cryptography
{
    enum class AESModes
    {
        ECB,
        CBC,
        CFB,
        OFB,
        CTR
    };

    template <size_t TLength>
    class AES
    {
    private:
        typedef const EVP_CIPHER *(*ModePointer)();

        Cryptography::Key _Key;
        Cryptography::Key _IV;

        ModePointer _Mode;

        inline static void HandleErrors()
        {
            throw std::runtime_error(ERR_reason_error_string(ERR_get_error()));
        }

        // @todo : Optimize this :/

        template <AESModes Mode>
        inline static constexpr ModePointer GetMode()
        {
            if constexpr (TLength == 128)
            {
                if constexpr (Mode == AESModes::ECB)
                {
                    return EVP_aes_128_ecb;
                }
                else if constexpr (Mode == AESModes::CBC)
                {
                    return EVP_aes_128_cbc;
                }
                else if constexpr (Mode == AESModes::CFB)
                {
                    return EVP_aes_128_cfb;
                }
                else if constexpr (Mode == AESModes::OFB)
                {
                    return EVP_aes_128_ofb;
                }
                else if constexpr (Mode == AESModes::CTR)
                {
                    return EVP_aes_128_ctr;
                }
            }
            else if constexpr (TLength == 192)
            {
                if constexpr (Mode == AESModes::ECB)
                {
                    return EVP_aes_192_ecb;
                }
                else if constexpr (Mode == AESModes::CBC)
                {
                    return EVP_aes_192_cbc;
                }
                else if constexpr (Mode == AESModes::CFB)
                {
                    return EVP_aes_192_cfb;
                }
                else if constexpr (Mode == AESModes::OFB)
                {
                    return EVP_aes_192_ofb;
                }
                else if constexpr (Mode == AESModes::CTR)
                {
                    return EVP_aes_192_ctr;
                }
            }
            else if constexpr (TLength == 256)
            {
                if constexpr (Mode == AESModes::ECB)
                {
                    return EVP_aes_256_ecb;
                }
                else if constexpr (Mode == AESModes::CBC)
                {
                    return EVP_aes_256_cbc;
                }
                else if constexpr (Mode == AESModes::CFB)
                {
                    return EVP_aes_256_cfb;
                }
                else if constexpr (Mode == AESModes::OFB)
                {
                    return EVP_aes_256_ofb;
                }
                else if constexpr (Mode == AESModes::CTR)
                {
                    return EVP_aes_256_ctr;
                }
            }
            else
            {
                throw std::runtime_error("Unsupported key length");
            }
        }

    public:
        static constexpr int BlockSize = 128 / 8;

        AES() = default;
        AES(Cryptography::Key Key, Cryptography::Key IV) : _Key(Key), _IV(IV)
        {
            // @todo : Check if key and IV are valid

            if ((Key.Size * 8) != TLength)
            {
                throw std::runtime_error("Key length must be " + std::to_string(TLength));
            }
        }

        inline Cryptography::Key Key() const
        {
            return _Key;
        }

        inline Cryptography::Key IV() const
        {
            return _IV;
        }

        inline Cryptography::Key &Key()
        {
            return _Key;
        }

        inline Cryptography::Key &IV()
        {
            return _IV;
        }

        // @todo Implement

        template <AESModes Mode>
        static constexpr size_t PlainSize(size_t CypherSize)
        {
            return CypherSize;
        }

        template <AESModes Mode>
        static constexpr size_t CypherSize(size_t PlainSize)
        {
            switch (Mode)
            {
            case AESModes::CFB:
            {
                return PlainSize;
            }
            case AESModes::OFB:
            {
                return PlainSize;
            }
            case AESModes::CTR:
            {
                return PlainSize;
            }
            // case AESModes::CCM:
            // {
            //     return PlainSize;
            // }
            default:
            {
                auto Temp = PlainSize % BlockSize;
                return Temp ? ((PlainSize - Temp) + BlockSize) : PlainSize;
            }
            }
        }

        template <AESModes TMode>
        int Encrypt(const unsigned char *Plain, int Size, unsigned char *Cypher) const
        {
            EVP_CIPHER_CTX *ctx;

            int len;

            int ciphertext_len;

            if (!(ctx = EVP_CIPHER_CTX_new()))
                HandleErrors();

            if (1 != EVP_EncryptInit_ex(
                         ctx,
                         (GetMode<TMode>())(),
                         NULL,
                         reinterpret_cast<const unsigned char *>(_Key.Data),
                         reinterpret_cast<const unsigned char *>(_IV.Data)))
                //  NULL))
                HandleErrors();

            if (1 != EVP_EncryptUpdate(ctx, Cypher, &len, Plain, Size))
                HandleErrors();

            ciphertext_len = len;

            if (1 != EVP_EncryptFinal_ex(ctx, Cypher + len, &len))
                HandleErrors();

            ciphertext_len += len;

            EVP_CIPHER_CTX_free(ctx);

            return ciphertext_len;
        }

        template <AESModes TMode>
        Iterable::Span<char> Encrypt(const unsigned char *Plain, int Size) const
        {
            Iterable::Span<char> Cypher(CypherSize<TMode>(Size));

            int len = Encrypt<TMode>(Plain, Size, reinterpret_cast<unsigned char *>(Cypher.Content()));

            if (static_cast<size_t>(len) != Cypher.Length())
            {
                Cypher.Resize(len);
            }

            return Cypher;
        }

        template <AESModes TMode>
        Iterable::Span<char> Encrypt(const Iterable::Span<char> &Plain) const
        {
            return Encrypt<TMode>(reinterpret_cast<const unsigned char *>(Plain.Content()), Plain.Length());
        }

        template <AESModes TMode>
        int Decrypt(const unsigned char *Cypher, int Size, unsigned char *Plain) const
        {
            EVP_CIPHER_CTX *ctx;

            int len;

            int ciphertext_len;

            if (!(ctx = EVP_CIPHER_CTX_new()))
                HandleErrors();

            if (1 != EVP_DecryptInit_ex(
                         ctx,
                         (GetMode<TMode>())(),
                         NULL,
                         reinterpret_cast<const unsigned char *>(_Key.Data),
                         reinterpret_cast<const unsigned char *>(_IV.Data)))
                //  NULL))
                HandleErrors();

            if (1 != EVP_DecryptUpdate(ctx, Plain, &len, Cypher, Size))
                HandleErrors();

            ciphertext_len = len;

            if (1 != EVP_DecryptFinal_ex(ctx, Plain + len, &len))
                HandleErrors();

            ciphertext_len += len;

            EVP_CIPHER_CTX_free(ctx);

            return ciphertext_len;
        }

        template <AESModes TMode>
        Iterable::Span<char> Decrypt(const unsigned char *Cypher, int Size) const
        {
            Iterable::Span<char> Plain(PlainSize<TMode>(Size));

            int len = Decrypt<TMode>(Cypher, Size, reinterpret_cast<unsigned char *>(Plain.Content()));

            if (static_cast<size_t>(len) != Plain.Length())
            {
                Plain.Resize(len);
            }

            return Plain;
        }

        template <AESModes TMode>
        Iterable::Span<char> Decrypt(const Iterable::Span<char> &Cypher) const
        {
            return Decrypt<TMode>(reinterpret_cast<const unsigned char *>(Cypher.Content()), Cypher.Length());
        }
    };
}