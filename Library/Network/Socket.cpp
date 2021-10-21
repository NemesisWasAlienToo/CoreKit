#pragma once

#include <iostream>
#include <functional>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>

#include "Network/EndPoint.cpp"
#include "Base/Descriptor.cpp"

#include "Iterable/Buffer.cpp"

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
                NonBlock = SOCK_NONBLOCK,
            };

            enum MessageFlags{
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
                _Handler = socket(_Protocol, _Type, 0);

                if (_Handler < 0)
                {
                    std::cout << "Error : " << strerror(errno) << std::endl;
                    exit(-1);
                }
            }

            Socket(const int Handler) : Descriptor(Handler) {}

            Socket(SocketFamily Protocol, int Type = TCP) : _Protocol(Protocol), _Type(Type)
            {
                _Handler = socket(Protocol, Type, 0);

                // Error handling here

                if (_Handler < 0)
                {
                    std::cout << "Init : " << strerror(errno) << std::endl;
                    exit(-1);
                }
            }

            Socket(const Socket &Other) : Descriptor(Other) {}

            bool Blocking(bool Value)
            {
                int Result = 0;

                int flags = fcntl(_Handler, F_GETFL, 0);

                if (!(((_Type & NonBlock) == 0) ^ Value))
                    return Value;

                flags ^= NonBlock;
                _Type ^= NonBlock;

                Result = fcntl(_Handler, F_SETFL, flags);

                // Error handling here

                if (Result < 0)
                {
                    std::cout << "Blocking : " << strerror(errno) << std::endl;
                    exit(-1);
                }

                return (_Type & NonBlock) == 0;
            }

            bool Blocking()
            {
                return (_Type & NonBlock) == 0;
            }

            void Bind(const EndPoint &Host)
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

                setsockopt(_Handler, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

                Result = bind(_Handler, SocketAddress, Size);

                // Error handling here

                if (Result < 0)
                {
                    std::cout << "Bind : " << strerror(errno) << std::endl;
                    exit(-1);
                }
            }

            void Bind(const Address &address, short Port)
            {
                Bind(EndPoint(address, Port));
            }

            void Connect(EndPoint Target)
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

                int Result = connect(_Handler, SocketAddress, Size);

                // Error handling here

                if (Result < 0 && errno != EINPROGRESS)
                {
                    std::cout << "Connect : " << strerror(errno) << std::endl;
                    exit(-1);
                }
            }

            void Connect(const Address &address, short Port)
            {
                Connect(EndPoint(address, Port));
            }

            void Listen(int Count)
            {
                int Result = listen(_Handler, Count);

                // Error handling here

                if (Result < 0)
                {
                    std::cout << "Listen : " << strerror(errno) << std::endl;
                    exit(-1);
                }
            }

            Socket Accept()
            {
                struct sockaddr_storage ClientAddress;
                socklen_t Size;
                int ClientDescriptor;

                Size = sizeof ClientAddress;
                ClientDescriptor = accept(_Handler, (struct sockaddr *)&ClientAddress, &Size);

                // Error handling here

                if (ClientDescriptor < 0)
                {
                    std::cout << "Accept : " << strerror(errno) << std::endl;
                    exit(-1);
                }

                return ClientDescriptor;
            }

            Socket Accept(EndPoint &Peer)
            {
                struct sockaddr_storage ClientAddress;
                socklen_t Size;
                int ClientDescriptor, Result = 0;

                Size = sizeof ClientAddress;
                ClientDescriptor = accept(_Handler, (struct sockaddr *)&ClientAddress, &Size);

                // Error handling here

                if (ClientDescriptor < 0 && errno != EAGAIN)
                {
                    std::cout << "Accept : " << strerror(errno) << std::endl;
                    exit(-1);
                }

                if (fcntl(_Handler, F_GETFL, 0) & O_NONBLOCK)
                    Result = fcntl(ClientDescriptor, F_SETFL, O_NONBLOCK);

                if (Result < 0)
                {
                    std::cout << "Accept : " << strerror(errno) << std::endl;
                    exit(-1);
                }

                Peer = EndPoint((struct sockaddr *)&ClientAddress);
                return ClientDescriptor;
            }

            bool Closable()
            {
                if (_Handler < 0)
                    return false;

                int error = 0;
                socklen_t len = sizeof(error);
                int retval = getsockopt(_Handler, SOL_SOCKET, SO_ERROR, &error, &len);

                if (retval != 0 || error != 0)
                {
                    std::cout << "Closable : " << strerror(errno) << std::endl;
                    return false;
                }

                return true;
            }

            EndPoint Peer()
            {
                struct sockaddr_storage addr;

                socklen_t addrlen = sizeof addr;

                int Result = getpeername(_Handler, (struct sockaddr *)&addr, &addrlen);

                if (Result < 0)
                {
                    std::cout << "Peer : " << strerror(errno) << std::endl;
                    exit(-1);
                }

                return (struct sockaddr *)&addr;
            }

            int Received()
            {

                int Count = 0;
                int Result = ioctl(_Handler, FIONREAD, &Count);

                if (Result < 0)
                {
                    std::cout << "Received : " << strerror(errno) << std::endl;
                    exit(-1);
                }

                return Count;
            }

            int Sending()
            {

                int Count = 0;
                int Result = ioctl(_Handler, TIOCOUTQ, &Count);

                if (Result < 0)
                {
                    std::cout << "Sending : " << strerror(errno) << std::endl;
                    exit(-1);
                }

                return Count;
            }

            Event Await(Event Events, int TimeoutMS = -1)
            {

                _POLLFD PollStruct = {.fd = _Handler, .events = Events};

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

            size_t Send(char *Data, size_t Length, int Flags)
            {
                return send(_Handler, Data, Length, Flags);
            }

            size_t Receive(char *Data, size_t Length, int Flags)
            {
                return recv(_Handler, Data, Length, Flags);
            }

            size_t SendTo(char *Data, size_t Length, EndPoint Target, int Flags = 0)
            {
                struct sockaddr_storage Client;
                socklen_t len = Target.sockaddr((struct sockaddr *)&Client);
                int Result = sendto(_Handler, (const char *)Data, Length, Flags, (struct sockaddr *)&Client, len);

                if (Result < 0)
                {
                    std::cout << "ReceiveFrom : " << strerror(errno) << std::endl;
                    exit(-1);
                }

                return Result;
            }

            size_t ReceiveFrom(char *Data, size_t Length, EndPoint &Target, int Flags = 0)
            {
                struct sockaddr_storage Client;
                socklen_t len = sizeof(Client);

                int Result = recvfrom(_Handler, Data, Length, Flags, (struct sockaddr *)&Client, &len);

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
                    Result = write(_Handler, str, Left);
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

            Socket &operator<<(Iterable::Buffer<char> &buffer)
            {
                size_t len = buffer.Length();

                char _Buffer[len];

                for (size_t i = 0; i < len; i++)
                {
                    _Buffer[i] = buffer[i];
                }

                int Result = write(_Handler, _Buffer, len);

                // Error handling here

                if (Result < 0)
                {
                    std::cout << "operator<< : " << strerror(errno) << std::endl;
                    exit(-1);
                }

                buffer.Free(Result);

                return *this;
            }

            Socket &operator>>(Iterable::Buffer<char> &buffer)
            {
                size_t len = buffer.Capacity() - buffer.Length();

                if (len == 0)
                    return *this;

                char _Buffer[len];

                int Result = read(_Handler, _Buffer, len);

                // Error handling here

                if (Result < 0)
                {
                    std::cout << "operator<< : " << strerror(errno) << std::endl;
                    exit(-1);
                }

                for (size_t i = 0; i < len; i++)
                {
                    buffer.Add(_Buffer[i]); // ## Optimize later
                }

                return *this;
            }

            Socket &operator>>(std::string &Message) noexcept
            {
                char buffer[1024];
                int Result = 0;

                while ((Result = read(_Handler, buffer, 1024)) > 0)
                {
                    buffer[Result] = 0;
                    Message.append(buffer);

                    if (Result < 1024)
                        break;
                }

                // Error handling here

                if (Result < 0 && errno != EAGAIN)
                {
                    std::cout << "operator>> " << errno << " : " << strerror(errno) << std::endl;
                    exit(-1);
                }

                return *this;
            }

            const Socket &operator>>(std::string &Message) const noexcept
            {
                char buffer[1024];
                int Result = 0;

                while ((Result = read(_Handler, buffer, 1024)) > 0)
                {
                    buffer[Result] = 0;
                    Message.append(buffer);

                    if (Result < 1024)
                        break;
                }

                // Error handling here

                if (Result < 0 && errno != EAGAIN)
                {
                    std::cout << "operator>> " << errno << " : " << strerror(errno) << std::endl;
                    exit(-1);
                }

                return *this;
            }

            // // Maybe add rvalue later?
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