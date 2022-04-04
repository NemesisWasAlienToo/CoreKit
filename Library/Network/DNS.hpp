#pragma once

#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <system_error>

#include "Network/EndPoint.hpp"
#include "Network/Socket.hpp"
#include "Network/Address.hpp"
#include "Iterable/List.hpp"
namespace Core
{
    namespace Network
    {
        namespace DNS
        {
            Iterable::List<EndPoint> Resolve(const std::string &Domain, const std::string &Service, Address::AddressFamily Family = Address::IPv4, Socket::SocketType Type = Socket::TCP)
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

            EndPoint ResolveSingle(const std::string &Domain, const std::string &Service, Address::AddressFamily Family = Address::IPv4, Socket::SocketType Type = Socket::TCP)
            {
                struct addrinfo hints, *res;
                int status;
                EndPoint endPoint{Network::Address{0}, 0};

                std::memset(&hints, 0, sizeof hints);
                hints.ai_family = Family;
                hints.ai_socktype = Type;
                // hints.ai_flags = 0; Maybe add later

                if ((status = getaddrinfo(Domain.c_str(), Service.c_str(), &hints, &res)) != 0)
                    return endPoint;

                if(res != NULL)
                {

                    endPoint = EndPoint(res->ai_addr);
                }

                freeaddrinfo(res);

                return endPoint;
            }

            Iterable::List<Address> Resolve(const std::string &Domain, Address::AddressFamily Family = Address::AnyFamily, Socket::SocketType Type = Socket::TCP)
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
                    if (p->ai_family == Network::Address::IPv4)
                    {
                        address = Address(((struct sockaddr_in *)res->ai_addr)->sin_addr);
                    }
                    else
                    {
                        address = Address(((struct sockaddr_in6 *)res->ai_addr)->sin6_addr);
                    }

                    addresses.Add(address);
                }

                freeaddrinfo(res);

                return addresses;
            }

            Address ResolveSingle(const std::string &Domain, Address::AddressFamily Family = Address::AnyFamily, Socket::SocketType Type = Socket::TCP)
            {
                struct addrinfo hints, *res;
                int status;
                Address address;

                std::memset(&hints, 0, sizeof hints);
                hints.ai_family = Family;
                hints.ai_socktype = Type;

                if ((status = getaddrinfo(Domain.c_str(), "", &hints, &res)) != 0)
                    return address;

                if(res != NULL)
                {
                    Address address;
                    if (res->ai_family == Network::Address::IPv4)
                    {
                        address = Address(((struct sockaddr_in *)res->ai_addr)->sin_addr);
                    }
                    else
                    {
                        address = Address(((struct sockaddr_in6 *)res->ai_addr)->sin6_addr);
                    }
                }

                freeaddrinfo(res);

                return address;
            }

            std::string Name(Address Target)
            {
                struct sockaddr_storage _Target;
                socklen_t len = Network::EndPoint(Target, 0).sockaddr_storage(&_Target);

                char host[NI_MAXHOST];

                int Result = getnameinfo((struct sockaddr *) &_Target, len, host, sizeof host, NULL, 0, 0);

                if( Result != 0){
                    throw std::system_error(errno, std::generic_category());
                }

                return host;
            }

            std::string Name(std::string Target)
            {
                return Name(Network::Address(Target));
            }

            std::string Service(EndPoint Target)
            {
                struct sockaddr_storage _Target;
                socklen_t len = Target.sockaddr_storage(&_Target);
                char serv[NI_MAXSERV];

                int Result = getnameinfo((struct sockaddr *)&_Target, len, NULL, 0, serv, sizeof serv, 0);

                if (Result != 0)
                {
                    throw std::system_error(errno, std::generic_category());
                }

                return serv;
            }

            Iterable::List<EndPoint> Host(const std::string &Service = "", Address::AddressFamily Family = Address::AnyFamily, Socket::SocketType Type = Socket::TCP, bool Passive = false)
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

            Iterable::List<Address> Host(Address::AddressFamily Family = Address::AnyFamily, Socket::SocketType Type = Socket::TCP, bool Passive = false)
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
                    if (p->ai_family == Network::Address::IPv4)
                    {
                        address = Address(((struct sockaddr_in *)p->ai_addr)->sin_addr);
                    }
                    else
                    {
                        address = Address(((struct sockaddr_in6 *)p->ai_addr)->sin6_addr);
                    }

                    addresses.Add(address);
                }

                freeaddrinfo(res);

                return addresses;
            }

            std::string HostName()
            {
                char name[40];
                gethostname(name, 40);
                return name;
            }
        };
    }
}