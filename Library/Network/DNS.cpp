#pragma once

#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#include "Network/Network.cpp"
#include "Network/EndPoint.cpp"

#include "Iterable/List.cpp"

namespace Network
{
    class DNS
    {
    private:
    public:
    
        DNS() = default;

        // Static

        static Iterable::List<EndPoint> Resolve(const std::string &Domain, const std::string &Service, AddressFamily Family, SocketType Type)
        {
            struct addrinfo hints, *res, *p;
            int status;
            Iterable::List<EndPoint> endPoints;

            std::memset(&hints, 0, sizeof hints);
            hints.ai_family = Family;
            hints.ai_socktype = Type;
            // hints.ai_flags = 0; Maybe add later

            if ((status = getaddrinfo(Domain.c_str(), Service.c_str(), &hints, &res)) != 0)
                return endPoints;

            for (p = res; p != NULL; p = p->ai_next)
            {

                EndPoint endPoint(p->ai_addr);

                endPoints.Add(endPoint);
            }

            freeaddrinfo(res);

            return endPoints;
        }

        static Iterable::List<Address> Resolve(const std::string &Domain, AddressFamily Family = IPv4Address, SocketType Type = TCP)
        {
            struct addrinfo hints, *res, *p;
            int status;
            Iterable::List<Address> addresses;

            std::memset(&hints, 0, sizeof hints);
            hints.ai_family = Family;
            hints.ai_socktype = Type;

            if ((status = getaddrinfo(Domain.c_str(), "", &hints, &res)) != 0)
                return addresses;

            for (p = res; p != NULL; p = p->ai_next)
            {
                Address address;
                if(p->ai_family == Network::IPv4Address){
                    address = Address(((struct sockaddr_in *) p->ai_addr)->sin_addr);
                }
                else{
                    address = Address(((struct sockaddr_in6 *) p->ai_addr)->sin6_addr);
                }

                addresses.Add(address);
            }

            freeaddrinfo(res);

            return addresses;
        }

        static Iterable::List<EndPoint> Host(const std::string &Service = "", AddressFamily Family = AnyFamilyAddress, SocketType Type = TCP, bool Passive = false)
        {
            struct addrinfo hints, *res, *p;
            int status;
            Iterable::List<EndPoint> endPoints;

            std::memset(&hints, 0, sizeof hints);
            hints.ai_family = Family;
            hints.ai_socktype = Type;
            hints.ai_flags = Passive ? AI_PASSIVE : AI_ALL;

            if ((status = getaddrinfo(NULL, Service.c_str(), &hints, &res)) != 0)
                return endPoints;

            for (p = res; p != NULL; p = p->ai_next)
            {
                EndPoint endPoint(p->ai_addr);

                endPoints.Add(endPoint);
            }

            freeaddrinfo(res);

            return endPoints;
        }

        static Iterable::List<Address> Host(AddressFamily Family = AnyFamilyAddress, SocketType Type = TCP, bool Passive = false)
        {
            struct addrinfo hints, *res, *p;
            int status;
            Iterable::List<Address> addresses;

            std::memset(&hints, 0, sizeof hints);
            hints.ai_family = Family;
            hints.ai_socktype = Type;
            hints.ai_flags = Passive ? AI_PASSIVE : AI_ALL;

            if ((status = getaddrinfo(NULL, "", &hints, &res)) != 0)
                return addresses;

            for (p = res; p != NULL; p = p->ai_next)
            {
                Address address;
                if(p->ai_family == Network::IPv4Address){
                    address = Address(((struct sockaddr_in *) p->ai_addr)->sin_addr);
                }
                else{
                    address = Address(((struct sockaddr_in6 *) p->ai_addr)->sin6_addr);
                }

                addresses.Add(address);
            }

            freeaddrinfo(res);

            return addresses;
        }

        static std::string HostName()
        {
            char name[40];
            gethostname(name, 40);
            return name;
        }
    };
}