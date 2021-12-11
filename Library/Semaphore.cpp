#pragma once

#include <iostream>
#include <unistd.h>
#include <poll.h>
#include <system_error>

namespace Core
{
    class Semaphore
    {
    public:

        // ### Constructors

        Semaphore()
        {
            
        }

        Semaphore(int Handler) : Descriptor(Handler) {}

        Semaphore(int Value, int Flags = 0)
        {
            
        }

        Semaphore(const Semaphore &Other) : Descriptor(Other) {}

        ~Semaphore() { Close(); }

        // ### Functionalitues

        void Emit(uint64_t Value) const
        {
            int Result = write(_INode, &Value, sizeof Value);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }
        }

        uint64_t Await(int TimeoutMS = -1) const
        {
            _POLLFD PollStruct = {.fd = _INode, .Semaphores = In};

            int Result = poll(&PollStruct, 1, TimeoutMS);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }
            else if (Result == 0)
            {
                return 0;
            }

            return Value();
        }

        // TryAwait

        uint64_t Value() const
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
