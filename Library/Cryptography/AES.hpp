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

namespace Core
{
    namespace Cryptography
    {
        enum class AESModes
            {
                ECB,
                CBC,
                CFB
            };

        template <AESModes TMode, size_t TLength>
        class AES
        {
        public:
            typedef const EVP_CIPHER *(*ModePointer)();

        private:
            Cryptography::Key _Key;
            Cryptography::Key _IV;

            ModePointer _Mode;

        public:
            AES() = default;
            AES(Cryptography::Key Key, Cryptography::Key IV) : _Key(Key), _IV(IV)
            {
                // @todo : Check if key and IV are valid
                // @todo : Optimize this :/

                if ((Key.Size * 8) != TLength)
                {
                    throw std::runtime_error("Key length must be " + std::to_string(TLength));
                }
                
                // if ((IV.Size * 8) != TLength)
                // {
                //     throw std::runtime_error("IV length must be " + std::to_string(TLength));
                // }

                if constexpr (TLength == 128)
                {
                    if constexpr (TMode == AESModes::ECB)
                    {
                        _Mode = EVP_aes_128_ecb;
                    }
                    else if constexpr (TMode == AESModes::CBC)
                    {
                        _Mode = EVP_aes_128_cbc;
                    }
                    else if constexpr (TMode == AESModes::CFB)
                    {
                        _Mode = EVP_aes_128_cfb;
                    }
                }
                else if constexpr (TLength == 192)
                {
                    if constexpr (TMode == AESModes::ECB)
                    {
                        _Mode = EVP_aes_192_ecb;
                    }
                    else if constexpr (TMode == AESModes::CBC)
                    {
                        _Mode = EVP_aes_192_cbc;
                    }
                    else if constexpr (TMode == AESModes::CFB)
                    {
                        _Mode = EVP_aes_192_cfb;
                    }
                }
                else if constexpr (TLength == 256)
                {
                    if constexpr (TMode == AESModes::ECB)
                    {
                        _Mode = EVP_aes_256_ecb;
                    }
                    else if constexpr (TMode == AESModes::CBC)
                    {
                        _Mode = EVP_aes_256_cbc;
                    }
                    else if constexpr (TMode == AESModes::CFB)
                    {
                        _Mode = EVP_aes_256_cfb;
                    }
                }
                else
                {
                    throw std::runtime_error("Unsupported key length");
                }
            }

            int PlainSize()
            {
                return TLength / 8;
            }

            int CypherSize()
            {
                return TLength / 8;
            }

            void handleErrors(void)
            {
                ERR_print_errors_fp(stderr);
                abort();
            }

            int Encrypt(const unsigned char *Plain, int Size, unsigned char *Cypher)
            {
                EVP_CIPHER_CTX *ctx;

                int len;

                int ciphertext_len;

                if (!(ctx = EVP_CIPHER_CTX_new()))
                    handleErrors();

                /*
                 * Initialise the encryption operation. IMPORTANT - ensure you use a key
                 * and IV size appropriate for your cipher
                 * In this example we are using 256 bit AES (i.e. a 256 bit key). The
                 * IV size for *most* modes is the same as the block size. For AES this
                 * is 128 bits
                 */
                if (1 != EVP_EncryptInit_ex(
                             ctx,
                             _Mode(),
                             NULL,
                             reinterpret_cast<const unsigned char *>(_Key.Data),
                             reinterpret_cast<const unsigned char *>(_IV.Data)))
                    handleErrors();

                if (1 != EVP_EncryptUpdate(ctx, Cypher, &len, Plain, Size))
                    handleErrors();

                ciphertext_len = len;

                /*
                 * Finalise the encryption. Further ciphertext bytes may be written at
                 * this stage.
                 */
                if (1 != EVP_EncryptFinal_ex(ctx, Cypher + len, &len))
                    handleErrors();
                
                ciphertext_len += len;

                EVP_CIPHER_CTX_free(ctx);

                return ciphertext_len;
            }

            Iterable::Span<char> Encrypt(const unsigned char *Plain, int Size)
            {
                Iterable::Span<char> Cypher(CypherSize());

                int len = Encrypt(Plain, Size, reinterpret_cast<unsigned char *>(Cypher.Content()));

                if(len != CypherSize())
                {
                    Cypher.Resize(len);
                }

                return Cypher;
            }

            Iterable::Span<char> Encrypt(const Iterable::Span<char>& Plain)
            {
                return Encrypt(reinterpret_cast<const unsigned char *>(Plain.Content()), Plain.Length());
            }

            int Decrypt(const unsigned char *Cypher, int Size, unsigned char *Plain)
            {
                EVP_CIPHER_CTX *ctx;

                int len;

                int ciphertext_len;

                if (!(ctx = EVP_CIPHER_CTX_new()))
                    handleErrors();

                /*
                 * Initialise the encryption operation. IMPORTANT - ensure you use a key
                 * and IV size appropriate for your cipher
                 * In this example we are using 256 bit AES (i.e. a 256 bit key). The
                 * IV size for *most* modes is the same as the block size. For AES this
                 * is 128 bits
                 */
                if (1 != EVP_DecryptInit_ex(
                             ctx,
                             _Mode(),
                             NULL,
                             reinterpret_cast<const unsigned char *>(_Key.Data),
                             reinterpret_cast<const unsigned char *>(_IV.Data)))
                    handleErrors();

                if (1 != EVP_DecryptUpdate(ctx, Plain, &len, Cypher, Size))
                    handleErrors();

                ciphertext_len = len;

                /*
                 * Finalise the encryption. Further ciphertext bytes may be written at
                 * this stage.
                 */
                if (1 != EVP_DecryptFinal_ex(ctx, Plain + len, &len))
                    handleErrors();
                
                ciphertext_len += len;

                EVP_CIPHER_CTX_free(ctx);

                return ciphertext_len;
            }

            Iterable::Span<char> Decrypt(const unsigned char *Cypher, int Size)
            {
                Iterable::Span<char> Plain(PlainSize());

                int len = Decrypt(Cypher, Size, reinterpret_cast<unsigned char *>(Plain.Content()));

                if(len != PlainSize())
                {
                    Plain.Resize(len);
                }

                return Plain;
            }

            Iterable::Span<char> Decrypt(const Iterable::Span<char>& Cypher)
            {
                return Decrypt(reinterpret_cast<const unsigned char *>(Cypher.Content()), Cypher.Length());
            }
        };
    }
}