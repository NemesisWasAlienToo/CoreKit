#pragma once

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/sendfile.h>
#include <sys/uio.h>
#include <algorithm>

#include <Iterable/Span.hpp>

namespace Core
{
    class Descriptor
    {
    protected:
        // Types :

        typedef struct pollfd _POLLFD;

        int _INode = -1;

    public:
        typedef short Event;

        enum DescriptorEvents
        {
            In = POLLIN,
            Out = POLLOUT,
            Error = POLLNVAL,
        };

        // ### Constructors

        Descriptor() = default;

        Descriptor(int Handler) : _INode(Handler) {}

        Descriptor(Descriptor const &) = delete;
        Descriptor(Descriptor &&Other) noexcept
        {
            std::swap(_INode, Other._INode);

            Other.Close();
        }

        virtual ~Descriptor()
        {
            Close();
        }

        // ### Functionalitues

        inline bool IsValid() const
        {
            return fcntl(_INode, F_GETFD) != -1 || errno != EBADF;
        }

        int Received() const
        {

            int Count = 0;
            int Result = ioctl(_INode, FIONREAD, &Count);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return Count;
        }

        ssize_t Write(struct iovec *Vector, size_t Count)
        {
            ssize_t Result = writev(_INode, Vector, Count);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return Result;
        }

        ssize_t Write(const void *Data, size_t Size) const
        {
            ssize_t Result = write(_INode, Data, Size);

            if (Result < 0)
            {
                auto EB = errno;

                if (EB == EAGAIN)
                    return 0;

                throw std::system_error(EB, std::generic_category());
            }

            return Result;
        }

        ssize_t Write(Iterable::Span<char> const &Data) const
        {
            ssize_t Result = write(_INode, Data.Content(), Data.Length());

            if (Result < 0)
            {
                auto EB = errno;

                if (EB == EAGAIN)
                    return 0;

                throw std::system_error(EB, std::generic_category());
            }

            return Result;
        }

        ssize_t Read(struct iovec *Vector, size_t Count) const
        {
            ssize_t Result = readv(_INode, Vector, Count);

            if (Result < 0)
            {
                auto EB = errno;

                if (EB == EAGAIN)
                    return 0;

                throw std::system_error(EB, std::generic_category());
            }

            return Result;
        }

        ssize_t Read(void *Data, size_t Size) const
        {
            ssize_t Result = read(_INode, Data, Size);

            if (Result < 0)
            {
                auto EB = errno;

                if (EB == EAGAIN)
                    return 0;

                throw std::system_error(EB, std::generic_category());
            }

            return Result;
        }

        ssize_t Read(Iterable::Span<char> &Data) const
        {
            ssize_t Result = read(_INode, Data.Content(), Data.Length());

            if (Result < 0)
            {
                auto EB = errno;

                if (EB == EAGAIN)
                    return 0;

                throw std::system_error(EB, std::generic_category());
            }

            return Result;
        }

        Iterable::Span<char> Read(size_t Size) const
        {
            Iterable::Span<char> Data(Size);

            ssize_t Result = read(_INode, Data.Content(), Data.Length());

            if (Result < 0)
            {
                auto EB = errno;

                if (EB == EAGAIN)
                    Iterable::Span<char>();

                throw std::system_error(EB, std::generic_category());
            }

            if (static_cast<size_t>(Result) != Data.Length())
            {
                Data.Resize(Result);
            }

            return Data;
        }

        void Blocking(bool Value)
        {
            int Result = 0;

            int flags = fcntl(_INode, F_GETFL, 0);

            if (((flags & O_NONBLOCK) == 0) != Value)
                flags ^= O_NONBLOCK;

            Result = fcntl(_INode, F_SETFL, flags);

            // Error handling here

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }
        }

        bool IsBlocking() const
        {
            return (fcntl(_INode, F_GETFL, 0) & O_NONBLOCK) == 0;
        }

        Event Await(Event Events, int TimeoutMS = -1) const
        {
            _POLLFD PollStruct = {.fd = _INode, .events = Events, .revents = 0};

            int Result = poll(&PollStruct, 1, TimeoutMS);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }
            else if (Result == 0)
            {
                return 0;
            }

            return PollStruct.revents;
        }

        void Close()
        {
            if (_INode < 0)
                return;

            int Result = close(_INode);

            // Error handling here

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            _INode = -1;
        }

        ssize_t SendFile(Descriptor const &Other, size_t Size, off_t Offset) const
        {
            int Result = sendfile(Other._INode, _INode, &Offset, Size);

            // Error handling here

            if (Result < 0)
            {
                auto EB = errno;

                if (EB == EAGAIN)
                    return 0;

                throw std::system_error(EB, std::generic_category());
            }

            return Result;
        }

        ssize_t SendFile(Descriptor const &Other, size_t Size) const
        {
            int Result = sendfile(_INode, Other._INode, nullptr, Size);

            // Error handling here

            if (Result < 0)
            {
                auto EB = errno;

                if (EB == EAGAIN)
                    return 0;

                throw std::system_error(EB, std::generic_category());
            }

            return Result;
        }

        // ### Peroperties

        inline int INode() const { return _INode; }

        Descriptor &operator=(Descriptor &&Other) noexcept
        {
            if (this != &Other)
            {
                std::swap(_INode, Other._INode);
                Other.Close();
            }

            return *this;
        }

        Descriptor &operator=(Descriptor const &Other) = delete;

        // ### Operators

        inline bool operator==(const Descriptor &Other) const noexcept
        {
            return _INode == Other._INode;
        }

        inline bool operator!=(const Descriptor &Other) const noexcept
        {
            return _INode != Other._INode;
        }

        inline bool operator>(const Descriptor &Other) const noexcept
        {
            return _INode > Other._INode;
        }

        inline bool operator<(const Descriptor &Other) const noexcept
        {
            return _INode < Other._INode;
        }

        // Stream operators
    };
}

namespace std
{
    template <>
    struct hash<Core::Descriptor>
    {
        size_t operator()(Core::Descriptor const &descriptor) const
        {
            return hash<int32_t>()(descriptor.INode());
        }
    };
}