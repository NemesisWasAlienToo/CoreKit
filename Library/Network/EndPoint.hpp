#pragma once

#include <sys/socket.h>

#include "Network/Address.hpp"

namespace Core
{
    namespace Network
    {
        class EndPoint
        {
        private:
            // Common

            Network::Address _Address;
            uint16_t _Port = 0;

            // IPv6 Specific

            uint32_t _Flow = 0;
            uint32_t _Scope = 0;

        public:
            // Constructors :

            EndPoint() = default;

            EndPoint(struct sockaddr *SocketAddress)
            {
                if (SocketAddress->sa_family == Address::IPv4)
                {
                    struct sockaddr_in addressStruct = *((struct sockaddr_in *)SocketAddress);
                    _Address.Set(addressStruct);
                    _Port = addressStruct.sin_port;
                }
                else
                {
                    struct sockaddr_in6 addressStruct = *((struct sockaddr_in6 *)SocketAddress);
                    _Address.Set(addressStruct);
                    _Port = addressStruct.sin6_port;
                    _Flow = addressStruct.sin6_flowinfo;
                    _Scope = addressStruct.sin6_scope_id;
                }
            }

            EndPoint(struct sockaddr_storage *SocketAddress)
            {
                if (SocketAddress->ss_family == Address::IPv4)
                {
                    struct sockaddr_in addressStruct = *((struct sockaddr_in *)SocketAddress);
                    _Address.Set(addressStruct);
                    _Port = addressStruct.sin_port;
                }
                else
                {
                    struct sockaddr_in6 addressStruct = *((struct sockaddr_in6 *)SocketAddress);
                    _Address.Set(addressStruct);
                    _Port = addressStruct.sin6_port;
                    _Flow = addressStruct.sin6_flowinfo;
                    _Scope = addressStruct.sin6_scope_id;
                }
            }

            EndPoint(struct sockaddr_in &SocketAddress) : _Address(SocketAddress.sin_addr)
            {
                _Port = SocketAddress.sin_port;
            }

            EndPoint(struct sockaddr_in6 &SocketAddress) : _Address(SocketAddress.sin6_addr)
            {
                _Port = SocketAddress.sin6_port;
                _Flow = SocketAddress.sin6_flowinfo;
                _Scope = SocketAddress.sin6_scope_id;
            }

            EndPoint(const Address &EndPointAddress, unsigned short Port) : _Address(EndPointAddress)
            {
                _Port = htons(Port);
            }

            EndPoint(const EndPoint &Othere) : _Address(Othere._Address), _Port(Othere._Port), _Flow(Othere._Flow), _Scope(Othere._Scope) {}

            EndPoint(const std::string &address, unsigned short Port)
            {
                _Address = Network::Address(address);
                _Port = htons(Port);
            }

            EndPoint(const std::string &Content)
            {
                int Seprator = Content.find(':');

                std::string IP = Content.substr(0, Seprator);
                std::string Port = Content.substr(Seprator + 1);

                _Address = Network::Address(IP);
                _Port = htons(std::stoi(Port));
            }

            // Functions :

            int sockaddr(struct sockaddr *SocketAddress) const
            {
                if (_Address.Family() == Address::IPv4)
                {
                    return sockaddr_in((struct sockaddr_in *)SocketAddress);
                }
                else
                {
                    return sockaddr_in6((struct sockaddr_in6 *)SocketAddress);
                }
            }

            int sockaddr_storage(struct sockaddr_storage *SocketAddress) const
            {
                if (_Address.Family() == Address::IPv4)
                {
                    return sockaddr_in((struct sockaddr_in *)SocketAddress);
                }
                else
                {
                    return sockaddr_in6((struct sockaddr_in6 *)SocketAddress);
                }
            }

            int sockaddr_in(struct sockaddr_in *SocketAddress) const
            {
                std::memset(SocketAddress, 0, sizeof(struct sockaddr_in));
                _Address.in_addr(&(SocketAddress->sin_addr));
                SocketAddress->sin_family = (sa_family_t)_Address.Family();
                SocketAddress->sin_port = _Port;
                return sizeof(struct sockaddr_in);
            }

            int sockaddr_in6(struct sockaddr_in6 *SocketAddress) const
            {
                std::memset(SocketAddress, 0, sizeof(struct sockaddr_in6));
                _Address.in6_addr(&(SocketAddress->sin6_addr));
                SocketAddress->sin6_family = (sa_family_t)_Address.Family();
                SocketAddress->sin6_port = _Port;
                SocketAddress->sin6_flowinfo = _Flow;
                SocketAddress->sin6_scope_id = _Scope;
                return sizeof(struct sockaddr_in6);
            }

            std::string ToString() const
            {
                return _Address.ToString() + ":" + std::to_string(ntohs(_Port));
            }

            // Properties

            //  Setters

            Network::Address &Address() { return _Address; }
            Network::Address Address() const { return _Address; }

            unsigned short Port() const { return _Port; }
            unsigned short Port(unsigned short Port)
            {
                _Port = Port;
                return _Port;
            }

            int Flow() const { return _Flow; }
            int Flow(int Flow)
            {
                _Flow = Flow;
                return _Flow;
            }

            int Scope() const { return _Scope; }
            int Scope(int Scope)
            {
                _Scope = Scope;
                return _Scope;
            }

            //  Setters

            void Set(Network::Address address, int Port, int Scope, int Flow)
            {
                _Address = address;
                _Port = Port;
                _Scope = Scope;
                _Flow = Flow;
            }

            // Operators

            // Maybe add scope and flow later?

            inline bool operator>(const EndPoint &Other) const
            {
                if (_Address > Other._Address)
                {
                    return true;
                }
                else if (_Address < Other._Address)
                {
                    return false;
                }
                else
                {
                    if (_Port <= Other._Port)
                        return false;
                }

                return true;
            }

            inline bool operator<(const EndPoint &Other) const
            {
                if (_Address > Other._Address)
                {
                    return false;
                }
                else if (_Address < Other._Address)
                {
                    return true;
                }
                else
                {
                    if (_Port >= Other._Port)
                        return false;
                }

                return true;
            }

            inline bool operator==(const EndPoint &Other) const
            {
                if (_Address == Other._Address && _Port == Other._Port)
                    return true;
                else
                    return false;
            }

            inline bool operator!=(const EndPoint &Other) const
            {
                if (_Address != Other._Address || _Port != Other._Port)
                    return true;
                else
                    return false;
            }

            inline bool operator>=(const EndPoint &Other) const
            {
                if (_Address >= Other._Address && _Port >= Other._Port)
                    return true;
                else
                    return false;
            }

            inline bool operator<=(const EndPoint &Other) const
            {
                if (_Address > Other._Address || _Port > Other._Port)
                    return false;
                else
                    return true;
            }

            friend std::ostream &operator<<(std::ostream &os, const EndPoint &tc)
            {
                return os << tc._Address << ":" << ntohs(tc._Port);
            }
        };
    }
}

namespace std
{
    template<>
    struct hash<Core::Network::EndPoint>
    {
        size_t operator()(const Core::Network::EndPoint &EndPoint) const
        {
            return hash<Core::Network::Address>()(EndPoint.Address()) ^ hash<unsigned short>()(EndPoint.Port());
        }
    };
}