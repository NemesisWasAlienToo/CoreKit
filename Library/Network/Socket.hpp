#pragma once

#include <iostream>
#include <functional>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <system_error>
#include <tuple>

#include "Descriptor.hpp"
#include "Network/EndPoint.hpp"
#include "Iterable/Queue.hpp"

// @todo Error handling for send, recv, sendto, recvfrom

namespace Core
{
    namespace Network
    {
        class Socket : public Descriptor
        {
        public:
            static constexpr size_t MaxConnections = SOMAXCONN;
            enum SocketFamily
            {
                Unspecified = PF_UNSPEC,
                Local = PF_LOCAL,
                Bluetooth = PF_BLUETOOTH,
                NFC = PF_NFC,
                CAN = PF_CAN,
                Unix = PF_UNIX,
                IPv4 = PF_INET,
                IPv6 = PF_INET6,
            };

            enum SocketType
            {
                TCP = SOCK_STREAM,
                UDP = SOCK_DGRAM,
                Raw = SOCK_RAW,
                NonBlocking = SOCK_NONBLOCK,
            };

            enum SocketMessage
            {
                CloseOnExec = MSG_CMSG_CLOEXEC,
                DontWait = MSG_DONTWAIT,
                ErrorQueue = MSG_ERRQUEUE,
                OutOfBand = MSG_OOB,
                Peek = MSG_PEEK,
                Truncate = MSG_TRUNC,
                WaitAll = MSG_WAITALL,
            };

            Socket() = default;

            Socket(const int INode) : Descriptor(INode) {}

            Socket(SocketFamily Family, int Type, int Protocol = 0)
            {
                _INode = socket(Family, Type, Protocol);

                // @todo Error handling here

                if (_INode < 0)
                {
                    throw std::system_error(errno, std::generic_category());
                }
            }

            Socket(Socket &&Other) noexcept : Descriptor(std::move(Other)) {}

            void Bind(const EndPoint &Host) const
            {

                struct sockaddr *SocketAddress;
                int Size = 0, Result = 0, yes = 1;

                if (Host.Address().Family() == Address::IPv4)
                {
                    SocketAddress = (struct sockaddr *)new struct sockaddr_in;
                    Size = Host.sockaddr_in((struct sockaddr_in *)SocketAddress);
                }
                else
                {
                    SocketAddress = (struct sockaddr *)new struct sockaddr_in6;
                    Size = Host.sockaddr_in6((struct sockaddr_in6 *)SocketAddress);
                }

                setsockopt(_INode, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

                Result = bind(_INode, SocketAddress, Size);

                // @todo Error handling here

                if (Result < 0)
                {
                    throw std::system_error(errno, std::generic_category());
                }
            }

            void Bind(const Address &address, unsigned short Port) const
            {
                Bind(EndPoint(address, Port));
            }

            void Connect(EndPoint Target) const
            {

                struct sockaddr *SocketAddress;
                int Size = 0;

                if (Target.Address().Family() == Address::IPv4)
                {
                    SocketAddress = (struct sockaddr *)new struct sockaddr_in;
                    Size = Target.sockaddr_in((struct sockaddr_in *)SocketAddress);
                }
                else
                {
                    SocketAddress = (struct sockaddr *)new struct sockaddr_in6;
                    Size = Target.sockaddr_in6((struct sockaddr_in6 *)SocketAddress);
                }

                int Result = connect(_INode, SocketAddress, Size);

                // Error handling here

                if (Result < 0)
                {
                    throw std::system_error(errno, std::generic_category());
                }
            }

            void Connect(const Address &address, short Port) const
            {
                Connect(EndPoint(address, Port));
            }

            void Listen(int Count = MaxConnections) const
            {
                int Result = listen(_INode, Count);

                // Error handling here

                if (Result < 0)
                {
                    throw std::system_error(errno, std::generic_category());
                }
            }

            std::tuple<Socket, EndPoint> Accept(int Flags = 0) const
            {
                struct sockaddr_storage ClientAddress;
                socklen_t Size;
                int ClientDescriptor;

                Size = sizeof ClientAddress;
                ClientDescriptor = accept4(_INode, (struct sockaddr *)&ClientAddress, &Size, Flags);

                // Error handling here

                if (ClientDescriptor < 0)
                {
                    throw std::system_error(errno, std::generic_category());
                }

                return {ClientDescriptor, (struct sockaddr *)&ClientAddress};
            }

            Socket Accept(EndPoint &Peer, int Flags = 0) const
            {
                struct sockaddr_storage ClientAddress;
                socklen_t Size;
                int ClientDescriptor;

                Size = sizeof ClientAddress;
                ClientDescriptor = accept4(_INode, (struct sockaddr *)&ClientAddress, &Size, Flags);

                // Error handling here

                if (ClientDescriptor < 0 /* && errno != EAGAIN*/)
                {
                    throw std::system_error(errno, std::generic_category());
                }

                Peer = EndPoint((struct sockaddr *)&ClientAddress);

                return ClientDescriptor;
            }

            // @todo Implement these

            // NoDelay option

            // int SetOptions()
            // int GetOptions()
            // {
            //     int error = 0;
            //     socklen_t len = sizeof(error);
            // int retval = getsockopt(_INode, SOL_SOCKET, SO_ERROR, &error, &len);

            //     if (retval != 0)
            //     {
            //         throw std::system_error(errno, std::generic_category());
            //     }

            //     return error;
            // }

            int Errors() const
            {
                int error = 0;
                socklen_t len = sizeof(error);
                int retval = getsockopt(_INode, SOL_SOCKET, SO_ERROR, &error, &len);

                if (retval != 0)
                {
                    throw std::system_error(errno, std::generic_category());
                }

                return error;
            }

            bool IsValid() const
            {
                if (_INode < 0 || Errors() != 0)
                    return false;

                return true;
            }

            EndPoint Peer() const
            {
                struct sockaddr_storage addr;

                socklen_t addrlen = sizeof addr;

                int Result = getpeername(_INode, (struct sockaddr *)&addr, &addrlen);

                if (Result < 0)
                {
                    throw std::system_error(errno, std::generic_category());
                }

                return (struct sockaddr *)&addr;
            }

            size_t Received() const
            {

                int Count = 0;
                int Result = ioctl(_INode, TIOCINQ, &Count);

                if (Result < 0)
                {
                    throw std::system_error(errno, std::generic_category());
                }

                return Count;
            }

            size_t Sending() const
            {

                int Count = 0;
                int Result = ioctl(_INode, TIOCOUTQ, &Count);

                if (Result < 0)
                {
                    throw std::system_error(errno, std::generic_category());
                }

                return Count;
            }

            size_t ReceiveBufferSize() const
            {
                unsigned int Size = 0;
                socklen_t len = sizeof Size;

                int Result = getsockopt(_INode, SOL_SOCKET, SO_RCVBUF, &Size, &len);

                if (Result < 0)
                {
                    throw std::system_error(errno, std::generic_category());
                }

                return Size;
            }

            size_t SendBufferSize() const
            {
                unsigned int Size = 0;
                socklen_t len = sizeof Size;

                int Result = getsockopt(_INode, SOL_SOCKET, SO_SNDBUF, &Size, &len);

                if (Result < 0)
                {
                    throw std::system_error(errno, std::generic_category());
                }

                return Size;
            }

            ssize_t Send(const char *Data, size_t Length, int Flags = 0) const
            {
                // @todo Fix return type or return result of send

                auto Result = send(_INode, Data, Length, Flags);

                if (Result < 0)
                {
                    if (errno == EAGAIN)
                    {
                        return 0;
                    }
                    else
                    {
                        throw std::system_error(errno, std::generic_category());
                    }
                }

                return Result;
            }

            ssize_t Send(const Iterable::Span<char> &Data, int Flags = 0) const
            {
                auto Result = send(_INode, Data.Content(), Data.Length(), Flags);

                if (Result < 0)
                {
                    if (errno == EAGAIN)
                    {
                        return 0;
                    }
                    else
                    {
                        throw std::system_error(errno, std::generic_category());
                    }
                }

                return Result;
            }

            ssize_t Receive(char *Data, size_t Length, int Flags = 0) const
            {
                // @todo Fix return type or return result of recv

                auto Result = recv(_INode, Data, Length, Flags);

                if (Result < 0)
                {
                    if (errno == EAGAIN)
                    {
                        return 0;
                    }
                    else
                    {
                        throw std::system_error(errno, std::generic_category());
                    }
                }

                return Result;
            }

            ssize_t Receive(Iterable::Span<char> &Data, int Flags = 0) const
            {
                auto Result = recv(_INode, Data.Content(), Data.Length(), Flags);

                if (Result < 0)
                {
                    if (errno == EAGAIN)
                    {
                        return 0;
                    }
                    else
                    {
                        throw std::system_error(errno, std::generic_category());
                    }
                }

                return Result;
            }

            Iterable::Span<char> Receive(int Flags = 0) const
            {
                Iterable::Span<char> Data(Received());

                auto Result = recv(_INode, Data.Content(), Data.Length(), Flags);

                if (Result < 0)
                {
                    if (errno == EAGAIN)
                    {
                        return 0;
                    }
                    else
                    {
                        throw std::system_error(errno, std::generic_category());
                    }
                }

                return Data;
            }

            ssize_t SendTo(const char *Data, size_t Length, const EndPoint &Target, int Flags = 0) const
            {
                struct sockaddr_storage Client = {0};
                socklen_t len = Target.sockaddr((struct sockaddr *)&Client);

                Network::EndPoint a((struct sockaddr *)&Client);

                int Result = sendto(_INode, (const char *)Data, Length, Flags, (struct sockaddr *)&Client, len);

                if (Result < 0)
                {
                    if (errno == EAGAIN)
                    {
                        return 0;
                    }
                    else
                    {
                        // @todo Add description from errno to exception

                        throw std::system_error(errno, std::generic_category());
                    }
                }

                return Result;
            }

            ssize_t SendTo(const Iterable::Span<char> &Data, const EndPoint &Target, int Flags = 0) const
            {
                struct sockaddr_storage Client = {0};
                socklen_t len = Target.sockaddr((struct sockaddr *)&Client);

                Network::EndPoint a((struct sockaddr *)&Client);

                int Result = sendto(_INode, Data.Content(), Data.Length(), Flags, (struct sockaddr *)&Client, len);

                if (Result < 0)
                {
                    if (errno == EAGAIN)
                    {
                        return 0;
                    }
                    else
                    {
                        throw std::system_error(errno, std::generic_category());
                    }
                }

                return Result;
            }

            ssize_t ReceiveFrom(char *Data, size_t Length, EndPoint &Target, int Flags = 0) const
            {
                struct sockaddr_storage Client;
                socklen_t len = sizeof(Client);

                ssize_t Result = recvfrom(_INode, Data, Length, Flags, (struct sockaddr *)&Client, &len);

                Target = EndPoint(&Client);

                if (Result < 0)
                {
                    if (errno == EAGAIN)
                    {
                        return 0;
                    }
                    else
                    {
                        throw std::system_error(errno, std::generic_category());
                    }
                }

                return Result;
            }

            ssize_t ReceiveFrom(Iterable::Span<char> &Data, EndPoint &Target, int Flags = 0) const
            {
                struct sockaddr_storage Client;
                socklen_t len = sizeof(Client);

                ssize_t Result = recvfrom(_INode, Data.Content(), Data.Length(), Flags, (struct sockaddr *)&Client, &len);

                Target = EndPoint(&Client);

                if (Result < 0)
                {
                    if (errno == EAGAIN)
                    {
                        return 0;
                    }
                    else
                    {
                        throw std::system_error(errno, std::generic_category());
                    }
                }

                return Result;
            }

            Iterable::Span<char> ReceiveFrom(EndPoint &Target, int Flags = 0) const
            {
                Iterable::Span<char> Data(Received());

                struct sockaddr_storage Client;
                socklen_t len = sizeof(Client);

                ssize_t Result = recvfrom(_INode, Data.Content(), Data.Length(), Flags, (struct sockaddr *)&Client, &len);

                Target = EndPoint(&Client);

                if (Result < 0)
                {
                    if (errno == EAGAIN)
                    {
                        return 0;
                    }
                    else
                    {
                        throw std::system_error(errno, std::generic_category());
                    }
                }

                return Data;
            }

            std::tuple<Iterable::Span<char>, EndPoint> ReceiveFrom(int Flags = 0) const
            {
                Iterable::Span<char> Data(Received());
                EndPoint Target;

                struct sockaddr_storage Client;
                socklen_t len = sizeof(Client);

                ssize_t Result = recvfrom(_INode, Data.Content(), Data.Length(), Flags, (struct sockaddr *)&Client, &len);

                Target = EndPoint(&Client);

                if (Result < 0)
                {
                    if (errno == EAGAIN)
                    {
                        return {0, Target};
                    }
                    else
                    {
                        throw std::system_error(errno, std::generic_category());
                    }
                }

                return std::tuple(std::move(Data), Target);
            }

            inline bool operator==(const Socket &Other) const
            {
                return Other._INode == _INode;
            }

            inline bool operator!=(const Socket &Other) const
            {
                return Other._INode != _INode;
            }

            Socket &operator=(Socket &&Other) noexcept
            {
                Descriptor::operator=(std::move(Other));

                return *this;
            }

            Socket &operator=(const Socket &Other) = delete;
        };
    }
}