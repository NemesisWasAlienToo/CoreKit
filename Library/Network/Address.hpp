#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <netdb.h>
#include <arpa/inet.h>

namespace Core
{
    namespace Network
    {
        class Address
        {
        public:
            // Types :

            enum AddressFamily
            {
                AnyFamily = AF_UNSPEC,
                // Local = AF_LOCAL,
                // Bluetooth = AF_BLUETOOTH,
                // NFCAddress = AF_NFC,
                // CAN = AF_CAN,
                Unix = AF_UNIX,
                IPv4 = AF_INET,
                IPv6 = AF_INET6,
            };

        private:
            typedef struct in_addr _IN_ADDR;
            typedef struct in6_addr _IN6_ADDR;
            typedef struct sockaddr_in _SOCKADDR_IN;
            typedef struct sockaddr_in6 _SOCKADDR_IN6;
            typedef struct sockaddr_storage _SOCKADDR_STORAGE;

            AddressFamily _Family = AnyFamily;
            unsigned char _Content[16] = {0};

        public:
            // Variables :

            // Constructors :

            Address() = default;

            Address(const uint32_t &Other) noexcept
            {
                _Family = IPv4;
                std::memcpy(_Content, &Other, sizeof Other);
            }

            Address(const _IN_ADDR &Other) noexcept
            {
                _Family = IPv4;
                std::memcpy(_Content, &(Other.s_addr), sizeof Other.s_addr);
            }

            Address(const _IN6_ADDR &Other) noexcept
            {
                _Family = IPv6;
                std::memcpy(_Content, &(Other.__in6_u), sizeof Other.__in6_u);
            }

            Address(AddressFamily Family, const std::string &Address) // noexcept
            {
                _Family = Family;
                int Result = inet_pton(Family, Address.c_str(), _Content);

                if (Result <= 0)
                    throw std::invalid_argument("Provided address is not valid");
            }

            Address(const std::string &Address) // noexcept
            {
                int Result = inet_pton(IPv4, Address.c_str(), _Content);

                if (Result > 0)
                {
                    _Family = IPv4;
                    return;
                }

                Result = inet_pton(IPv6, Address.c_str(), _Content);

                if (Result > 0)
                {
                    _Family = IPv6;
                    return;
                }
                else
                {
                    throw std::invalid_argument("Provided address is not valid");
                }
            }

            Address(const Address &Other) noexcept
            {
                _Family = Other._Family;
                std::memcpy(_Content, Other._Content, sizeof _Content);
            }

            // Functions :

            void Set(AddressFamily Family, std::string IPAddress)
            {
                _Family = Family;
                inet_pton(_Family, IPAddress.c_str(), _Content);
            }

            void Set(AddressFamily Family)
            {
                _Family = Family;
            }

            void Set(std::string IPAddress)
            {
                inet_pton(_Family, IPAddress.c_str(), _Content);
            }

            void Set(_SOCKADDR_IN Address)
            {
                _Family = (AddressFamily)Address.sin_family;
                std::memcpy(_Content, &(Address.sin_addr.s_addr), sizeof Address.sin_addr.s_addr);
            }

            void Set(_SOCKADDR_IN6 Address)
            {
                _Family = (AddressFamily)Address.sin6_family;
                std::memcpy(_Content, &(Address.sin6_addr.__in6_u), sizeof Address.sin6_addr.__in6_u);
            }

            // Properties :

            inline AddressFamily Family() const { return _Family; }

            std::string IP() const
            {
                char str[INET6_ADDRSTRLEN] = {0};
                inet_ntop(_Family, _Content, str, sizeof str);
                return str;
            }

            int in_addr(_IN_ADDR *Address)
            {
                std::memcpy(Address, _Content, sizeof(_IN_ADDR));
                return sizeof(_IN_ADDR);
            }

            int in_addr(_IN_ADDR *Address) const
            {
                std::memcpy(Address, _Content, sizeof(_IN_ADDR));
                return sizeof(_IN_ADDR);
            }

            int in6_addr(_IN6_ADDR *Address)
            {
                std::memcpy(Address, _Content, sizeof(_IN6_ADDR));
                return sizeof(_IN6_ADDR);
            }

            int in6_addr(_IN6_ADDR *Address) const
            {
                std::memcpy(Address, _Content, sizeof(_IN6_ADDR));
                return sizeof(_IN6_ADDR);
            }

            std::string ToString() const
            {
                char str[INET6_ADDRSTRLEN] = {0};
                inet_ntop(_Family, (void *)_Content, str, sizeof str);
                return std::string(str);
            }

            // Operators :

            inline bool operator>(const Address &Other) const
            {
                for (int i = 0; i <= 15; i++)
                {
                    if (_Content[i] > Other._Content[i])
                    {
                        return true;
                    }
                    else if(_Content[i] < Other._Content[i])
                    {
                        return false;
                    }
                }

                return false;
            }

            inline bool operator<(const Address &Other) const
            {
                for (int i = 0; i <= 15; i++)
                {
                    if (_Content[i] < Other._Content[i])
                    {
                        return true;
                    }
                    else if(_Content[i] > Other._Content[i])
                    {
                        return false;
                    }
                }

                return false;
            }

            inline bool operator==(const Address &Other) const
            {
                for (int i = 0; i <= 15; i++)
                {
                    if (_Content[i] != Other._Content[i])
                    {
                        return false;
                    }
                }

                return true;
            }

            inline bool operator!=(const Address &Other) const
            {
                for (int i = 0; i <= 15; i++)
                {
                    if (_Content[i] != Other._Content[i])
                    {
                        return true;
                    }
                }

                return false;
            }

            inline bool operator>=(const Address &Other) const
            {
                for (int i = 0; i <= 15; i++)
                {
                    if (_Content[i] < Other._Content[i])
                    {
                        return false;
                    }
                }

                return true;
            }

            inline bool operator<=(const Address &Other) const
            {
                for (int i = 0; i <= 15; i++)
                {
                    if (_Content[i] > Other._Content[i])
                    {
                        return false;
                    }
                }

                return true;
            }

            friend std::ostream &operator<<(std::ostream &os, const Address &tc)
            {
                char str[INET6_ADDRSTRLEN] = {0};
                inet_ntop(tc._Family, (void *)tc._Content, str, sizeof str);
                return os << str;
            }

            // Statics :

            static Address Any(AddressFamily Family = IPv4)
            {
                if (Family == IPv4)
                    return (uint32_t)INADDR_ANY;
                else if (Family == IPv6)
                    return (_IN6_ADDR)IN6ADDR_ANY_INIT;
                else
                    throw std::invalid_argument(""); // For future uses
            }

            static Address Loop(AddressFamily Family = IPv4)
            {
                if (Family == IPv4)
                    return (uint32_t)INADDR_LOOPBACK;
                else if (Family == IPv6)
                    return (_IN6_ADDR)IN6ADDR_LOOPBACK_INIT;
                else
                    throw std::invalid_argument(""); // For future uses
            }

            // Optimize this

            static Address FromString(AddressFamily Family, std::string AddressString)
            {
                Address adr(Family, AddressString);
                return adr;
            }
        };
    }
}