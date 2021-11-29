#pragma once

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <poll.h>
#include <sys/eventfd.h>

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

        Descriptor(const Descriptor &Other) : _INode(Other._INode) {}

        // ### Functionalitues

        auto Write(const void *Data, size_t Size)
        {
            return write(_INode, Data, Size);
        }

        auto Read(void *Data, size_t Size)
        {
            return read(_INode, Data, Size);
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

        bool Blocking() const
        {
            return (fcntl(_INode, F_GETFL, 0) & O_NONBLOCK) == 0;
        }

        virtual Event Await(Event Events, int TimeoutMS = -1) const
        {

            _POLLFD PollStruct = {.fd = _INode, .events = Events};

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

        // ### Peroperties

        int INode() const { return _INode; }

        Descriptor &operator=(Descriptor &Other) = delete;

        Descriptor &operator=(Descriptor &&Other)
        {
            std::swap(_INode, Other._INode);
            return *this;
        }

        // ### Operators

        inline bool operator==(const Descriptor &Other) const
        {
            return _INode == Other._INode;
        }

        inline bool operator!=(const Descriptor &Other) const
        {
            return _INode != Other._INode;
        }

        inline bool operator>(const Descriptor &Other) const
        {
            return _INode > Other._INode;
        }

        inline bool operator<(const Descriptor &Other) const
        {
            return _INode < Other._INode;
        }
    };
}
