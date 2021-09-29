#pragma once

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

namespace Base
{
    class Descriptor
    {
    protected:
        int _Handler = -1;

    public:

        typedef short Event;

        enum DescriptorEvents{
            In = POLLIN,
            Out = POLLOUT,
            Error = POLLNVAL,
        };

        Event Events;

        Descriptor() = default;
        Descriptor(int Handler) : _Handler(Handler) {}

        void Close()
        {
            if (_Handler < 0)
                return;

            int Result = close(_Handler);
            _Handler = -1;

            // Error handling here

            if (Result < 0)
            {
                std::cout << "Error :" << strerror(errno) << std::endl;
                exit(-1);
            }
        }

        int Handler() { return _Handler; }
    };
}