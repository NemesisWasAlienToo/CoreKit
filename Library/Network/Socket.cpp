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

#include "Descriptor.cpp"
#include "Network/EndPoint.cpp"
#include "Iterable/Queue.cpp"

// To do :
//      - Error handling
//      - const functions

namespace Core
{
    namespace Network
    {
        class Socket : public Descriptor
        {
        public:
            enum SocketFamily
            {
                Any = PF_UNSPEC,
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

            Socket()
            {
                _INode = socket(IPv4, TCP, 0);

                if (_INode < 0)
                {
                    throw std::system_error(errno, std::generic_category());
                }
            }

            Socket(const int Handler) : Descriptor(Handler) {}

            Socket(SocketFamily Protocol, int Type = TCP)
            {
                _INode = socket(Protocol, Type, 0);

                // Error handling here

                if (_INode < 0)
                {
                    throw std::system_error(errno, std::generic_category());
                }
            }

            Socket(const Socket &Other) : Descriptor(Other) {}

            Socket(const Descriptor &Other) : Descriptor(Other) {}

            void Bind(const EndPoint &Host) const
            {

                struct sockaddr *SocketAddress;
                int Size = 0, Result = 0, yes = 1;

                if (Host.address().Family() == Address::IPv4)
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

                // Error handling here

                if (Result < 0)
                {
                    throw std::system_error(errno, std::generic_category());
                }
            }

            void Bind(const Address &address, short Port) const
            {
                Bind(EndPoint(address, Port));
            }

            void Connect(EndPoint Target) const
            {

                struct sockaddr *SocketAddress;
                int Size = 0;

                if (Target.address().Family() == Address::IPv4)
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

            bool IsConnected() const
            {
                char Data;
                int Result = Receive(&Data, 1, DontWait | Peek);

                if ((Result == 0) || (Result < 0 && errno != EAGAIN))
                {
                    return false;
                }

                return true;
            }

            void Listen(int Count) const
            {
                int Result = listen(_INode, Count);

                // Error handling here

                if (Result < 0)
                {
                    throw std::system_error(errno, std::generic_category());
                }
            }

            std::tuple<Socket, EndPoint> Accept() const
            {
                struct sockaddr_storage ClientAddress;
                socklen_t Size;
                int ClientDescriptor;

                Size = sizeof ClientAddress;
                ClientDescriptor = accept(_INode, (struct sockaddr *)&ClientAddress, &Size);

                // Error handling here

                if (ClientDescriptor < 0)
                {
                    throw std::system_error(errno, std::generic_category());
                }

                return {ClientDescriptor, (struct sockaddr *)&ClientAddress};
            }

            Socket Accept(EndPoint &Peer) const
            {
                struct sockaddr_storage ClientAddress;
                socklen_t Size;
                int ClientDescriptor;

                Size = sizeof ClientAddress;
                ClientDescriptor = accept(_INode, (struct sockaddr *)&ClientAddress, &Size);

                // Error handling here

                if (ClientDescriptor < 0 /* && errno != EAGAIN*/)
                {
                    throw std::system_error(errno, std::generic_category());
                }

                Peer = EndPoint((struct sockaddr *)&ClientAddress);

                // @todo : The descriptor will not have NonBlocking flag in its type

                Socket Ret(ClientDescriptor);

                if (!IsBlocking()) Ret.Blocking(false);

                return Ret;
            }

            bool Closable() const
            {
                if (_INode < 0)
                    return false;

                int error = 0;
                socklen_t len = sizeof(error);
                int retval = getsockopt(_INode, SOL_SOCKET, SO_ERROR, &error, &len);

                if (retval != 0 || error != 0)
                {
                    throw std::system_error(errno, std::generic_category());
                }

                return true;
            }

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
                return send(_INode, Data, Length, Flags);
            }

            ssize_t Send(const Iterable::Span<char> &Data, int Flags = 0) const
            {
                return send(_INode, Data.Content(), Data.Length(), Flags);
            }

            ssize_t Receive(char *Data, size_t Length, int Flags = 0) const
            {
                return recv(_INode, Data, Length, Flags);
            }

            ssize_t Receive(Iterable::Span<char> &Data, int Flags = 0) const
            {
                return recv(_INode, Data.Content(), Data.Length(), Flags);
            }

            ssize_t SendTo(const char *Data, size_t Length, const EndPoint &Target, int Flags = 0) const
            {
                struct sockaddr_storage Client = {0, 0};
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
                        throw std::system_error(errno, std::generic_category());
                    }
                }

                return Result;
            }

            ssize_t SendTo(const Iterable::Span<char> &Data, const EndPoint &Target, int Flags = 0) const
            {
                struct sockaddr_storage Client = {0, 0};
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

            Socket &operator<<(const std::string &Message)
            {
                int Result = 0;
                int Left = Message.length();
                const char *str = Message.c_str();

                while (Left > 0)
                {
                    Await(Out);
                    Result = write(_INode, str, Left);
                    Left -= Result;
                }

                // Error handling here

                if (Result < 0)
                {
                    throw std::system_error(errno, std::generic_category());
                }

                return *this;
            }

            const Socket &operator<<(const std::string &Message) const
            {
                int Result = 0;
                int Left = Message.length();
                const char *str = Message.c_str();

                while (Left > 0)
                {
                    Await(Out);
                    Result = write(_INode, str, Left);
                    Left -= Result;
                }

                // Error handling here

                if (Result < 0)
                {
                    throw std::system_error(errno, std::generic_category());
                }

                return *this;
            }

            const Socket &operator<<(Iterable::Queue<char> &queue) const
            {
                // @todo Clarify on blocking and non-blocking

                int Result;

                for (;;)
                {
                    auto [Data, Size] = queue.Chunk();

                    if (Size == 0)
                    {
                        break;
                    }

                    Result = write(_INode, Data, Size);

                    if (Result > 0)
                    {
                        queue.Free(Result);
                    }
                    else
                    {
                        break;
                    }
                }

                // Error handling here

                if (Result < 0 && errno != EAGAIN)
                {
                    throw std::system_error(errno, std::generic_category());
                }

                return *this;
            }

            Socket &operator<<(Iterable::Queue<char> &queue)
            {
                // @todo Clarify on blocking and non-blocking

                int Result;

                for (;;)
                {
                    auto [Data, Size] = queue.Chunk();

                    if (Size == 0)
                    {
                        break;
                    }

                    Result = write(_INode, Data, Size);

                    if (Result > 0)
                    {
                        queue.Free(Result);
                    }
                    else
                    {
                        break;
                    }
                }

                // Error handling here

                if (Result < 0 && errno != EAGAIN)
                {
                    throw std::system_error(errno, std::generic_category());
                }

                return *this;
            }

            const Socket &operator>>(Iterable::Queue<char> &queue) const
            {
                size_t Size = Received();

                if (Size <= 0)
                    return *this;

                char _Buffer[Size];

                int Result = read(_INode, _Buffer, Size);

                // Error handling here

                if (Result < 0)
                {
                    throw std::system_error(errno, std::generic_category());
                }

                queue.Add(_Buffer, Size);

                return *this;
            }

            Socket &operator>>(Iterable::Queue<char> &queue)
            {
                size_t Size = Received();

                if (Size <= 0)
                    return *this;

                char _Buffer[Size];

                int Result = read(_INode, _Buffer, Size);

                // Error handling here

                if (Result < 0)
                {
                    throw std::system_error(errno, std::generic_category());
                }

                queue.Add(_Buffer, Size);

                return *this;
            }

            Socket &operator>>(std::string &Message)
            {
                size_t Size = Received() + 1;

                if (Size <= 0)
                    return *this;

                char buffer[Size];

                int Result = read(_INode, buffer, Size);

                // Instead, can increase string size by
                // avalable bytes in socket buffer
                // and call read on c_str at the new memory index

                if (Result < 0 && errno != EAGAIN)
                {
                    throw std::system_error(errno, std::generic_category());
                }

                buffer[Result] = 0;
                Message.append(buffer);

                return *this;
            }

            const Socket &operator>>(std::string &Message) const
            {
                size_t Size = Received() + 1;

                if (Size <= 0)
                    return *this;

                char buffer[Size];

                int Result = read(_INode, buffer, Size);

                // Instead, can increase string size by
                // avalable bytes in socket buffer
                // and call read on c_str at the new memory index

                if (Result < 0 && errno != EAGAIN)
                {
                    throw std::system_error(errno, std::generic_category());
                }

                buffer[Result] = 0;
                Message.append(buffer);

                return *this;
            }

            inline bool operator==(const Socket &Other)
            {
                return Other._INode == _INode;
            }

            inline bool operator!=(const Socket &Other)
            {
                return Other._INode != _INode;
            }

            // Maybe add rvalue later?

            Socket &operator=(const Socket &Other) = delete;

            // Friend operators

            friend std::ostream &operator<<(std::ostream &os, const Socket &socket)
            {
                std::string str;
                socket >> str; // fix this
                return os << str;
            }
        };
    }
}