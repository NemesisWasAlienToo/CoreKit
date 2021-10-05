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

            // _SOCKADDR_STORAGE _AddressStorage;
            AddressFamily family = AnyFamily;
            unsigned char address[INET6_ADDRSTRLEN] = {0}; //16

        public:

            // Variables :

            // Constructors :

            static Address Any(AddressFamily Family = AnyFamily){
                if(Family == IPv4) return INADDR_ANY;
                else if(Family == IPv6) return IN6ADDR_ANY_INIT;
                else return INADDR_ANY; // For future uses
            }

            static Address Loop(AddressFamily Family = AnyFamily){
                if(Family == IPv4) return INADDR_LOOPBACK;
                else if(Family == IPv6) return IN6ADDR_LOOPBACK_INIT;
                else return INADDR_LOOPBACK; // For future uses
            }

            Address() = default;

            Address(const uint32_t &Other) noexcept
            {
                family = IPv4;
                std::memcpy(address, &Other, sizeof Other);
            }

            Address(const _IN_ADDR &Other) noexcept
            {
                family = IPv4;
                std::memcpy(address, &(Other.s_addr), sizeof Other.s_addr);
            }

            Address(const _IN6_ADDR &Other) noexcept
            {
                family = IPv6;
                std::memcpy(address, &(Other.__in6_u), sizeof Other.__in6_u);
            }

            Address(AddressFamily Family, std::string Address) noexcept
            {
                family = Family;
                inet_pton(Family, Address.c_str(), address);
            }

            Address(const Address &Other) noexcept
            {
                family = Other.family;
                std::memcpy(address, Other.address, sizeof address);
            }

            // Functions :

            void Set(AddressFamily Family, std::string IPAddress)
            {
                family = Family;
                inet_pton(family, IPAddress.c_str(), address);
            }

            void Set(AddressFamily Family)
            {
                family = Family;
            }

            void Set(std::string IPAddress)
            {
                inet_pton(family, IPAddress.c_str(), address);
            }

            void Set(_SOCKADDR_IN Address)
            {
                family = (AddressFamily)Address.sin_family;
                std::memcpy(address, &(Address.sin_addr.s_addr), sizeof Address.sin_addr.s_addr);
            }

            void Set(_SOCKADDR_IN6 Address)
            {
                family = (AddressFamily)Address.sin6_family;
                std::memcpy(address, &(Address.sin6_addr.__in6_u), sizeof Address.sin6_addr.__in6_u);
            }

            // Properties :

            AddressFamily Family() { return family; }
            AddressFamily Family() const { return family; }

            std::string IP()
            {
                char str[INET6_ADDRSTRLEN] = {0};
                inet_ntop(family, address, str, sizeof str);
                return str;
            }

            std::string IP() const
            {
                char str[INET6_ADDRSTRLEN] = {0};
                inet_ntop(family, address, str, sizeof str);
                return str;
            }

            int in_addr(_IN_ADDR *Address)
            {
                std::memcpy(Address, address, sizeof(_IN_ADDR));
                return sizeof(_IN_ADDR);
            }

            int in6_addr(struct in6_addr *Address)
            {
                std::memcpy(Address, address, sizeof(struct in6_addr));
                return sizeof(struct in6_addr);
            }

            // Operators :

            friend std::ostream &operator<<(std::ostream &os, const Address &tc)
            {
                char str[INET6_ADDRSTRLEN] = {0};
                inet_ntop(tc.family, (void *)tc.address, str, sizeof str);
                return os << str;
            }

            // Statics :

            // Optimize this

            Address FromString(AddressFamily Family, std::string AddressString)
            {
                Address adr(Family, AddressString);
                return adr;
            }
        };
    }
}