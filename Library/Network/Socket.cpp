#pragma once

#include <iostream>
#include <functional>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>

#include "Network/Network.cpp"
#include "Network/EndPoint.cpp"
#include "Base/Descriptor.cpp"
#include "Base/Exeption.cpp"

// To do :
//      - Non blocking
//      - Error handling
//      - const functions
//      - Close on dispose

namespace Network
{
    class Socket : public Base::Descriptor
    {
    private:
        // Types :

        typedef struct pollfd _POLLFD;

        // Variables :

        ProtocolFamily _Protocol = IPv4Protocol;
        SocketType _Type = TCP;

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

        Socket(ProtocolFamily Protocol, SocketType Type = TCP, bool Blocking = true) : _Protocol(Protocol), _Type(Type)
        {
            int Result = 0;
            _Handler = socket(Protocol, Type, 0);

            // Error handling here

            if (_Handler < 0)
            {
                std::cout << "Init : " << strerror(errno) << std::endl;
                exit(-1);
            }

            if (!Blocking)
                Result = fcntl(_Handler, F_SETFL, O_NONBLOCK);

            // Error handling here

            if (Result < 0)
            {
                std::cout << "Init : " << strerror(errno) << std::endl;
                exit(-1);
            }
        }

        Socket(const Socket &Other)
        {
            _Handler = Other._Handler;
        }

        bool Blocking(bool Value)
        {
            int Result = 0;

            int flags = fcntl(_Handler, F_GETFL, 0);
            flags = Value ? flags ^ O_NONBLOCK : flags | O_NONBLOCK;

            Result = fcntl(_Handler, F_SETFL, flags);

            // Error handling here

            if (Result < 0)
            {
                std::cout << "Blocking : " << strerror(errno) << std::endl;
                exit(-1);
            }

            return flags & O_NONBLOCK;
        }

        bool Blocking()
        {
            int Result = fcntl(_Handler, F_GETFL, 0);

            return Result & O_NONBLOCK;
        }

        void Bind(EndPoint &Host)
        {

            struct sockaddr *SocketAddress;
            int Size = 0, Result = 0, yes = 1;

            if (Host.address().Family() == IPv4Address)
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

        void Connect(EndPoint Target)
        {

            struct sockaddr *SocketAddress;
            int Size = 0;

            if (Target.address().Family() == IPv4Address)
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

        int HasData()
        {

            int Count = 0;
            int Result = ioctl(_Handler, FIONREAD, &Count);

            if (Result < 0)
            {
                std::cout << "Peer : " << strerror(errno) << std::endl;
                return 0;
            }

            return Count;
        }

        Event Await(Event Events, int TimeoutMS = -1){

            _POLLFD PollStruct = {.fd = _Handler, .events = Events};

            int Result = poll(&PollStruct, 1, TimeoutMS);

            if (Result < 0)
            {
                std::cout << "Peer : " << strerror(errno) << std::endl;
                return 0;
            }

            return PollStruct.revents;
        }

        Socket &operator<<(const std::string &Message) noexcept
        {
            int Result = write(_Handler, Message.c_str(), Message.length());

            // Error handling here

            if (Result < 0)
            {
                std::cout << "operator<< : " << strerror(errno) << std::endl;
                exit(-1);
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

        inline bool operator==(const Socket &Other)
        {
            return Other._Handler == this->_Handler;
        }

        inline bool operator!=(const Socket &Other)
        {
            return Other._Handler != this->_Handler;
        }

        Socket &operator=(const Socket &Other) = delete;

        // Friend operators

        friend std::ostream &operator<<(std::ostream &os, const Socket &socket)
        {
            std::string str;
            socket >> str;
            return os << str;
        }
    };
}