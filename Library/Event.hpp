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

        Event() = default;

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

        Event(Event &&Other) noexcept : Descriptor(std::move(Other)) {}
        Event(Event const &Other) = delete;

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
            _POLLFD PollStruct = {.fd = _INode, .events = In, .revents = 0};

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

        Event &operator=(Event const &Other) = delete;

        Event &operator=(Event &&Other) noexcept
        {
            Descriptor::operator=(std::move(Other));

            return *this;
        }

        // ### Peroperties
    };
}
