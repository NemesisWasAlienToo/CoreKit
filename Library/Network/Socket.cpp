#pragma once

#include <iostream>
#include <functional>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>

#include "Base/Descriptor.cpp"
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

        private:
            // Types :

            typedef struct pollfd _POLLFD;

            // Variables :

            SocketFamily _Protocol = IPv4;
            int _Type = TCP;

        public:
            Socket()
            {
                _INode = socket(_Protocol, _Type, 0);

                if (_INode < 0)
                {
                    std::cout << "Error : " << strerror(errno) << std::endl;
                    exit(-1);
                }
            }

            Socket(const int Handler) : Descriptor(Handler) {}

            Socket(SocketFamily Protocol, int Type = TCP) : _Protocol(Protocol), _Type(Type)
            {
                _INode = socket(Protocol, Type, 0);

                // Error handling here

                if (_INode < 0)
                {
                    std::cout << "Init : " << strerror(errno) << std::endl;
                    exit(-1);
                }
            }

            Socket(const Socket &Other) : Descriptor(Other) {}

            Socket(const Descriptor &Other) : Descriptor(Other) {}

            bool Blocking(bool Value)
            {
                int Result = 0;

                int flags = fcntl(_INode, F_GETFL, 0);

                if (!(((_Type & NonBlocking) == 0) ^ Value))
                    return Value;

                flags ^= NonBlocking;
                _Type ^= NonBlocking;

                Result = fcntl(_INode, F_SETFL, flags);

                // Error handling here

                if (Result < 0)
                {
                    std::cout << "Blocking : " << strerror(errno) << std::endl;
                    exit(-1);
                }

                return (_Type & NonBlocking) == 0;
            }

            bool Blocking() const
            {
                return (_Type & NonBlocking) == 0;
            }

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
                    std::cout << "Bind : " << strerror(errno) << std::endl;
                    exit(-1);
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

                if (Result < 0 && errno != EINPROGRESS)
                {
                    throw std::invalid_argument(strerror(errno));
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
                    std::cout << "Listen : " << strerror(errno) << std::endl;
                    exit(-1);
                }
            }

            Socket Accept() const
            {
                struct sockaddr_storage ClientAddress;
                socklen_t Size;
                int ClientDescriptor;

                Size = sizeof ClientAddress;
                ClientDescriptor = accept(_INode, (struct sockaddr *)&ClientAddress, &Size);

                // Error handling here

                if (ClientDescriptor < 0)
                {
                    std::cout << "Accept : " << strerror(errno) << std::endl;
                    exit(-1);
                }

                return ClientDescriptor;
            }

            Socket Accept(EndPoint &Peer) const
            {
                struct sockaddr_storage ClientAddress;
                socklen_t Size;
                int ClientDescriptor, Result = 0;

                Size = sizeof ClientAddress;
                ClientDescriptor = accept(_INode, (struct sockaddr *)&ClientAddress, &Size);

                // Error handling here

                if (ClientDescriptor < 0 && errno != EAGAIN)
                {
                    std::cout << "Accept : " << strerror(errno) << std::endl;
                    exit(-1);
                }

                if (fcntl(_INode, F_GETFL, 0) & O_NONBLOCK)
                    Result = fcntl(ClientDescriptor, F_SETFL, O_NONBLOCK);

                if (Result < 0)
                {
                    std::cout << "Accept : " << strerror(errno) << std::endl;
                    exit(-1);
                }

                Peer = EndPoint((struct sockaddr *)&ClientAddress);
                return ClientDescriptor;
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
                    std::cout << "Closable : " << strerror(errno) << std::endl;
                    return false;
                }

                return true;
            }

            EndPoint Peer() const
            {
                struct sockaddr_storage addr;

                socklen_t addrlen = sizeof addr;

                int Result = getpeername(_INode, (struct sockaddr *)&addr, &addrlen);

                if (Result < 0)
                {
                    std::cout << "Peer : " << strerror(errno) << std::endl;
                    exit(-1);
                }

                return (struct sockaddr *)&addr;
            }

            int Received() const
            {

                int Count = 0;
                int Result = ioctl(_INode, FIONREAD, &Count);

                if (Result < 0)
                {
                    std::cout << "Received : " << strerror(errno) << std::endl;
                    exit(-1);
                }

                return Count;
            }

            int Sending() const
            {

                int Count = 0;
                int Result = ioctl(_INode, TIOCOUTQ, &Count);

                if (Result < 0)
                {
                    std::cout << "Sending : " << strerror(errno) << std::endl;
                    exit(-1);
                }

                return Count;
            }

            Event Await(Event Events, int TimeoutMS = -1) const
            {

                _POLLFD PollStruct = {.fd = _INode, .events = Events};

                int Result = poll(&PollStruct, 1, TimeoutMS);

                if (Result < 0)
                {
                    std::cout << "Await : " << strerror(errno) << std::endl;
                    return 0;
                }
                else if (Result == 0)
                {
                    return 0;
                }

                return PollStruct.revents;
            }

            ssize_t Send(char *Data, size_t Length, int Flags = 0) const
            {
                return send(_INode, Data, Length, Flags);
            }

            ssize_t Receive(char *Data, size_t Length, int Flags = 0) const
            {
                return recv(_INode, Data, Length, Flags);
            }

            ssize_t SendTo(char *Data, size_t Length, EndPoint Target, int Flags = 0) const
            {
                struct sockaddr_storage Client;
                socklen_t len = Target.sockaddr((struct sockaddr *)&Client);
                int Result = sendto(_INode, (const char *)Data, Length, Flags, (struct sockaddr *)&Client, len);

                if (Result < 0)
                {
                    std::cout << "ReceiveFrom : " << strerror(errno) << std::endl;
                    exit(-1);
                }

                return Result;
            }

            ssize_t ReceiveFrom(char *Data, size_t Length, EndPoint &Target, int Flags = 0) const
            {
                struct sockaddr_storage Client;
                socklen_t len = sizeof(Client);

                int Result = recvfrom(_INode, Data, Length, Flags, (struct sockaddr *)&Client, &len);

                if (Result < 0)
                {
                    std::cout << "ReceiveFrom : " << strerror(errno) << std::endl;
                    exit(-1);
                }

                Target = EndPoint(&Client);

                return Result;
            }

            Socket &operator<<(const std::string &Message) noexcept
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
                    std::cout << "operator<< : " << strerror(errno) << std::endl;
                    exit(-1);
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
                    std::cout << "operator<< : " << strerror(errno) << std::endl;
                    exit(-1);
                }

                return *this;
            }

            const Socket &operator<<(Iterable::Queue<char> &queue) const
            {
                auto _Buffer = queue.Chunk();

                int Result = write(_INode, _Buffer.Content(), _Buffer.Length());

                // Error handling here

                if (Result < 0)
                {
                    std::cout << "operator<< : " << strerror(errno) << std::endl;
                    exit(-1);
                }

                // ### Probably send more if can?

                queue.Free(Result);

                return *this;
            }

            Socket &operator<<(Iterable::Queue<char> &queue)
            {
                auto _Buffer = queue.Chunk();

                int Result = write(_INode, _Buffer.Content(), _Buffer.Length());

                // Error handling here

                if (Result < 0)
                {
                    std::cout << "operator<< : " << strerror(errno) << std::endl;
                    exit(-1);
                }

                // ### Probably send more if can?

                queue.Free(Result);

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
                    std::cout << "operator<< : " << strerror(errno) << std::endl;
                    exit(-1);
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
                    std::cout << "operator<< : " << strerror(errno) << std::endl;
                    exit(-1);
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
                    std::cout << "operator>> " << errno << " : " << strerror(errno) << std::endl;
                    exit(-1);
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
                    std::cout << "operator>> " << errno << " : " << strerror(errno) << std::endl;
                    exit(-1);
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