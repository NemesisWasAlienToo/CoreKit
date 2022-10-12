#pragma once

#include <string>
#include <system_error>
#include <Network/Socket.hpp>
#include <Format/Stream.hpp>
#include <openssl/ssl.h>
#include <openssl/err.h>

// enum class SSLError
// {
//     SSL = SSL_ERROR_SSL,
//     WantRead = SSL_ERROR_WANT_READ,
//     WantWrite = SSL_ERROR_WANT_WRITE,
// };

// namespace std
// {
//     template <>
//     struct is_error_condition_enum<SSLError> : public true_type
//     {
//     };
// }

// // custom category:
// class custom_category_t : public std::error_category
// {
// public:
//     virtual const char *name() const noexcept { return "openssl"; }

//     virtual std::string message(int ev) const
//     {
//         //
//     }
// } openssl_category;

// std::error_condition make_error_condition(SSLError e)
// {
//     return std::error_condition(static_cast<int>(e), openssl_category);
// }

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
                    throw std::runtime_error(GetErrorString());
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
                    ssl = nullptr;
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

            inline void ClearErrors()
            {
                ERR_clear_error();
            }

            inline int GetError(int Result) const
            {
                return SSL_get_error(ssl, Result);
            }

            char const *GetErrorString() const
            {
                return SSL_state_string(ssl);
            }

            ssize_t Write(void const *Data, size_t Size) const
            {
                ssize_t Result = SSL_write(ssl, Data, Size);

                if (Result < 0)
                {
                    throw std::runtime_error(GetErrorString());
                }

                return Result;
            }

            ssize_t Read(void *Data, size_t Size) const
            {
                ERR_clear_error();
                ssize_t Result = SSL_read(ssl, Data, Size);

                if (Result < 0)
                {
                    throw std::runtime_error(GetErrorString());
                }

                return Result;
            }

            ssize_t Write(Format::Stream &Stream)
            {
                ssize_t Sent = 0;
                ERR_clear_error();

                while (!Stream.Queue.IsEmpty())
                {
                    auto [Pointer, Size] = Stream.Queue.DataChunk();

                    auto Result = SSL_write(ssl, Pointer, Size);

                    if (Result <= 0)
                    {
                        int Error = SSL_get_error(ssl, Result);

                        if (Error == SSL_ERROR_WANT_WRITE)
                        {
                            return Sent;
                        }

                        return -1;

                        // throw std::runtime_error("SSL Failure : " + std::to_string(Error));
                    }

                    Sent += Result;
                    Stream.Queue.AdvanceHead(Result);
                }

                return Sent;
            }

            ssize_t Read(Format::Stream &Stream)
            {
                int Result = 0;
                size_t Size = 0;
                ssize_t GotBytes = 0;
                ERR_clear_error();

                do
                {
                    auto [Pointer, S] = Stream.Queue.EmptyChunk();
                    Size = S;

                    Result = SSL_read(ssl, Pointer, Size);

                    if (Result <= 0)
                    {
                        int Error = SSL_get_error(ssl, Result);

                        if (Error == SSL_ERROR_WANT_READ)
                        {
                            return GotBytes;
                        }

                        return -1;

                        // throw std::runtime_error("SSL Failure : " + std::to_string(Error));
                    }

                    GotBytes += Result;
                    Stream.Queue.AdvanceTail(Result);

                } while (static_cast<size_t>(Result) == Size);

                return GotBytes;
            }

            ssize_t SendFile(Descriptor const &descriptor, size_t Size, off_t Offset = 0) const
            {
                ERR_clear_error();
                int Result = SSL_sendfile(ssl, descriptor.INode(), Offset, Size, 0);

                // Error handling here

                if (Result < 0)
                {
                    throw std::runtime_error(GetErrorString());
                }

                return Result;
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