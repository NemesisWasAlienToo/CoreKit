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
        SSL_CTX *ctx = nullptr;

        TLSContext(std::string_view Certification, std::string_view Key)
        {
            ctx = Create();

            // @todo Fix Handle errors properly

            if (SSL_CTX_use_certificate_file(ctx, Certification.begin(), SSL_FILETYPE_PEM) <= 0)
            {
                ERR_print_errors_fp(stderr);
                exit(EXIT_FAILURE);
            }

            if (SSL_CTX_use_PrivateKey_file(ctx, Key.begin(), SSL_FILETYPE_PEM) <= 0)
            {
                ERR_print_errors_fp(stderr);
                exit(EXIT_FAILURE);
            }

            if (!SSL_CTX_check_private_key(ctx))
            {
                ERR_print_errors_fp(stderr);
                exit(EXIT_FAILURE);
            }
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

        static SSL_CTX *Create()
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
                perror("Unable to create SSL context");
                ERR_print_errors_fp(stderr);
                exit(EXIT_FAILURE);
            }

            SSL_CONF_CTX *cctx = SSL_CONF_CTX_new();
            SSL_CONF_CTX_set_flags(cctx, SSL_CONF_FLAG_SERVER);
            SSL_CONF_CTX_set_flags(cctx, SSL_CONF_FLAG_CLIENT);
            SSL_CONF_CTX_set_flags(cctx, SSL_CONF_FLAG_CERTIFICATE);
            SSL_CONF_CTX_set_flags(cctx, SSL_CONF_FLAG_FILE);
            SSL_CONF_CTX_set_ssl_ctx(cctx, ctx);
            SSL_CONF_CTX_finish(cctx);
            SSL_CONF_CTX_free(cctx);

            // SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

            return ctx;
        }
    };

    struct SecureSocket
    {
        SSL *ssl = nullptr;
        bool ShakeHand = false;

        SecureSocket() = default;

        SecureSocket(TLSContext const &Context, Network::Socket const &Socket) : ssl(SSL_new(Context.ctx))
        {
            if (!SSL_set_fd(ssl, Socket.INode()))
            {
                ERR_print_errors_fp(stderr);
                exit(EXIT_FAILURE);
            }

            SSL_set_verify(ssl, SSL_VERIFY_NONE, nullptr);

            SSL_set_accept_state(ssl);
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

        bool Handshake()
        {
            return (ShakeHand = (SSL_do_handshake(ssl) == 1));
        }

        ssize_t Write(void const *Data, size_t Size) const
        {
            ssize_t Result = SSL_write(ssl, Data, Size);

            if (Result < 0)
            {
                // @todo Fix this

                // throw std::system_error(errno, std::generic_category());

                ERR_print_errors_fp(stderr);
                exit(EXIT_FAILURE);
            }

            return Result;
        }

        ssize_t Read(void *Data, size_t Size) const
        {
            ssize_t Result = SSL_read(ssl, Data, Size);

            if (Result < 0)
            {
                // @todo Fix this

                // throw std::system_error(errno, std::generic_category());

                ERR_print_errors_fp(stderr);
                exit(EXIT_FAILURE);
            }

            return Result;
        }

        ssize_t SendFile(Descriptor const &Other, size_t Size, off_t Offset) const
        {
            int Result = SSL_sendfile(ssl, Other.INode(), Offset, Size, 0);

            // Error handling here

            if (Result < 0)
            {
                // @todo Fix this

                // throw std::system_error(errno, std::generic_category());

                ERR_print_errors_fp(stderr);
                exit(EXIT_FAILURE);
            }

            return Result;
        }

        friend SecureSocket &operator<<(SecureSocket &descriptor, Format::Stream &Stream)
        {
            while (!Stream.Queue.IsEmpty())
            {
                ERR_clear_error();
                
                auto [Pointer, Size] = Stream.Queue.DataChunk();

                auto Result = SSL_write(descriptor.ssl, Pointer, Size);

                if (Result <= 0)
                {
                    int Error = SSL_get_error(descriptor.ssl, Result);

                    if (Error != SSL_ERROR_WANT_WRITE && Error != SSL_ERROR_WANT_READ)
                    {
                        ERR_print_errors_fp(stderr);
                        exit(EXIT_FAILURE);
                    }

                    return descriptor;
                }

                Stream.Queue.AdvanceHead(Result);
            }

            return descriptor;
        }

        friend SecureSocket &operator>>(SecureSocket &descriptor, Format::Stream &Stream)
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
                        break;
                    }

                    ERR_print_errors_fp(stderr);
                    exit(EXIT_FAILURE);
                }

                Stream.Queue.AdvanceTail(Result);

            } while (static_cast<size_t>(Result) == Size);

            return descriptor;
        }

        inline operator bool()
        {
            return bool(ssl);
        }
    };
}