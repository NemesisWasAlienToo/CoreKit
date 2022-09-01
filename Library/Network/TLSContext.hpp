#pragma once

#include <string>
#include <mutex>
#include <Network/Socket.hpp>
#include <Format/Stream.hpp>
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace Core::Network
{
    struct TLSContext
    {
        static_assert(OPENSSL_VERSION_NUMBER >= 0x10100000L, "Least acceptable version of openssl is 1.1.0");

        struct SecureSocket
        {
            SSL *ssl = nullptr;
            bool ShakeHand = false;

            SecureSocket() = default;

            SecureSocket(SSL_CTX *ctx) : ssl(SSL_new(ctx)) {}

            inline void SetDescriptor(Descriptor const &descriptor)
            {
                int Result = SSL_set_fd(ssl, descriptor);

                if (!Result)
                {
                    throw std::runtime_error(std::to_string(GetError(Result)));
                }
            }

            inline void SetAccept()
            {
                SSL_set_accept_state(ssl);
            }

            inline void SetVerify(int mode, SSL_verify_cb Callback)
            {
                SSL_set_verify(ssl, mode, Callback);
            }

            SecureSocket(SecureSocket &&Other) : ssl(Other.ssl), ShakeHand(Other.ShakeHand)
            {
                Other.ssl = nullptr;
                Other.ShakeHand = false;
            }

            ~SecureSocket()
            {
                if (ssl)
                {
                    SSL_shutdown(ssl);
                    SSL_free(ssl);
                }
            }

            inline auto Handshake()
            {
                auto Res = SSL_do_handshake(ssl);
                ShakeHand = (Res == 1);
                return Res;
            }

            inline int Shutdown() const
            {
                return SSL_shutdown(ssl);
            }

            inline int GetError(int Result) const
            {
                return SSL_get_error(ssl, Result);
            }

            ssize_t Write(void const *Data, size_t Size) const
            {
                ssize_t Result = SSL_write(ssl, Data, Size);

                if (Result < 0)
                {
                    throw std::runtime_error(std::to_string(GetError(Result)));
                }

                return Result;
            }

            ssize_t Read(void *Data, size_t Size) const
            {
                ssize_t Result = SSL_read(ssl, Data, Size);

                if (Result < 0)
                {
                    throw std::runtime_error(std::to_string(GetError(Result)));
                }

                return Result;
            }

            ssize_t SendFile(Descriptor const &descriptor, size_t Size, off_t Offset = 0) const
            {
                int Result = SSL_sendfile(ssl, descriptor.INode(), Offset, Size, 0);

                // Error handling here

                if (Result < 0)
                {
                    throw std::runtime_error(std::to_string(GetError(Result)));
                }

                return Result;
            }

            friend bool operator<<(SecureSocket &descriptor, Format::Stream &Stream)
            {
                while (!Stream.Queue.IsEmpty())
                {
                    ERR_clear_error();

                    auto [Pointer, Size] = Stream.Queue.DataChunk();

                    auto Result = SSL_write(descriptor.ssl, Pointer, Size);

                    if (Result <= 0)
                    {
                        int Error = SSL_get_error(descriptor.ssl, Result);

                        if (Error == SSL_ERROR_WANT_WRITE)
                        {
                            return true;
                        }

                        return false;
                    }

                    Stream.Queue.AdvanceHead(Result);
                }

                return true;
            }

            friend bool operator>>(SecureSocket &descriptor, Format::Stream &Stream)
            {
                int Result = 0;
                size_t Size = 0;

                do
                {
                    Stream.Queue.IncreaseCapacity(1024);
                    auto [Pointer, S] = Stream.Queue.EmptyChunk();
                    Size = S;

                    Result = SSL_read(descriptor.ssl, Pointer, Size);

                    if (Result <= 0)
                    {
                        int Error = SSL_get_error(descriptor.ssl, Result);

                        if (Error == SSL_ERROR_WANT_READ)
                        {
                            return true;
                        }

                        return false;
                    }

                    Stream.Queue.AdvanceTail(Result);

                } while (static_cast<size_t>(Result) == Size);

                return true;
            }

            inline operator bool()
            {
                return bool(ssl);
            }
        };

        SSL_CTX *ctx = nullptr;

        TLSContext(std::string_view Certification, std::string_view Key)
        {
            ctx = Create();

            // @todo Fix Handle errors properly

            SetCertificate(Certification, SSL_FILETYPE_PEM);
            SetPrivateKey(Key, SSL_FILETYPE_PEM);
            CheckPrivateKey();
        }

        TLSContext(TLSContext &&Other) : ctx(Other.ctx)
        {
            Other.ctx = nullptr;
        }

        TLSContext &operator=(TLSContext &&Other)
        {
            ctx = Other.ctx;
            Other.ctx = nullptr;
            return *this;
        }

        ~TLSContext()
        {
            SSL_CTX_free(ctx);
        }

        inline void SetCertificate(std::string_view Certification, int Type = SSL_FILETYPE_PEM)
        {
            if (SSL_CTX_use_certificate_file(ctx, Certification.begin(), Type) <= 0)
            {
                throw std::runtime_error(ERR_reason_error_string(ERR_get_error()));
            }
        }

        inline void SetPrivateKey(std::string_view Key, int Type = SSL_FILETYPE_PEM)
        {
            if (SSL_CTX_use_PrivateKey_file(ctx, Key.begin(), Type) <= 0)
            {
                throw std::runtime_error(ERR_reason_error_string(ERR_get_error()));
            }
        }

        inline void CheckPrivateKey()
        {
            if (!SSL_CTX_check_private_key(ctx))
            {
                throw std::runtime_error(ERR_reason_error_string(ERR_get_error()));
            }
        }

        inline SecureSocket NewSocket()
        {
            return SecureSocket(ctx);
        }

        static SSL_CTX *Create(Iterable::List<std::pair<std::string, std::string>> const &Commands = {})
        {
            static std::once_flag once;

            std::call_once(
                once,
                []
                {
                    SSL_library_init();
                    ERR_load_crypto_strings();
                    SSL_load_error_strings();
                    OpenSSL_add_all_algorithms();
                });

            SSL_CTX *ctx = SSL_CTX_new(TLS_method());

            if (!ctx)
            {
                throw std::runtime_error(ERR_reason_error_string(ERR_get_error()));
            }

            SSL_CONF_CTX *confctx = SSL_CONF_CTX_new();
            SSL_CONF_CTX_set_flags(confctx, SSL_CONF_FLAG_SERVER);
            SSL_CONF_CTX_set_flags(confctx, SSL_CONF_FLAG_CERTIFICATE);
            SSL_CONF_CTX_set_flags(confctx, SSL_CONF_FLAG_FILE);
            SSL_CONF_CTX_set_ssl_ctx(confctx, ctx);
            Commands.ForEach(
                [&confctx](auto const &Pair)
                {
                    SSL_CONF_cmd(confctx, Pair.first.c_str(), Pair.second.c_str());
                });
            SSL_CONF_CTX_finish(confctx);
            SSL_CONF_CTX_free(confctx);

            SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

            return ctx;
        }
    };
}