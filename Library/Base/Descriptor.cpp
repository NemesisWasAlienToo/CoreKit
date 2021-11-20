#pragma once

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <poll.h>

namespace Core
{
    class Descriptor
    {
    protected:
        int _INode = -1;

    public:
        typedef short Event;

        enum DescriptorEvents
        {
            In = POLLIN,
            Out = POLLOUT,
            Error = POLLNVAL,
        };

        Event Events;

        Descriptor() = default;

        Descriptor(int Handler) : _INode(Handler) {}

        Descriptor(const Descriptor &Other) : _INode(Other._INode) {}

        void Close()
        {
            if (_INode < 0)
                return;

            int Result = close(_INode);

            // Error handling here

            if (Result < 0)
            {
                std::cout << "Error :" << strerror(errno) << std::endl;
                exit(-1);
            }
            
            _INode = -1;
        }

        int INode() const { return _INode; }

        Descriptor &operator=(Descriptor &Other) = delete;

        Descriptor &operator=(Descriptor &&Other)
        {
            std::swap(_INode, Other._INode);
            return *this;
        }

        inline bool operator==(const Descriptor &Other) const
        {
            return Other._INode == _INode;
        }

        inline bool operator!=(const Descriptor &Other) const
        {
            return Other._INode != _INode;
        }
    };
}
