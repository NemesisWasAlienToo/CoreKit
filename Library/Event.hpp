#pragma once

#include <iostream>
#include <unistd.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <system_error>

#include <Descriptor.hpp>

namespace Core
{
    class Event : public Descriptor
    {
    public:
        enum EventFlags
        {
            CloseOnExec = EFD_CLOEXEC,
            NonBlocking = EFD_NONBLOCK,
            Semaphore = EFD_SEMAPHORE,
        };

        // ### Constructors

        Event()
        {
            int Result = eventfd(0, 0);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            _INode = Result;
        }

        Event(int Handler) : Descriptor(Handler) {}

        Event(int Value, int Flags)
        {
            int Result = eventfd(Value, Flags);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            _INode = Result;
        }

        Event(const Event &Other) : Descriptor(Other) {}

        ~Event() { Close(); }

        // ### Functionalitues

        void Emit(uint64_t Value) const
        {
            int Result = write(_INode, &Value, sizeof Value);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }
        }

        int Await(int TimeoutMS = -1) const
        {
            _POLLFD PollStruct = {.fd = _INode, .events = In};

            int Result =  poll(&PollStruct, 1, TimeoutMS);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return Result;
        }

        // TryAwait

        uint64_t Listen() const
        {
            uint64_t Value;

            int Result = read(_INode, &Value, sizeof Value);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return Value;
        }

        // ### Peroperties
    };
}
