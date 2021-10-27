#pragma once

#include <sys/epoll.h>

#include "Iterable/Iterable.cpp"
#include "Network/Socket.cpp"
#include "Base/Descriptor.cpp"

namespace Core
{
    namespace Iterable
    {
        struct CEPoll
        {
            uint32_t Events;
            uint64_t Data;

            void Descriptor(int Value){
                *((uint64_t *) &Data) = Value;
            }
        };

        class ePoll
        {
        private:
        int a = sizeof(void*);
            /* data */
        public:
            ePoll(/* args */) {}
            ~ePoll() {}
        };
    }
}