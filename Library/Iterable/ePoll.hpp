#pragma once

#include <sys/epoll.h>

#include "Iterable/Iterable.hpp"
#include "Network/Socket.hpp"
#include "Descriptor.hpp"

namespace Core
{
    namespace Iterable
    {
        struct CEPoll
        {
            uint32_t Events = 0;
            uint64_t Data = static_cast<uint64_t>(-1);

            int Descriptor() const
            {
                return *((int32_t *)&Data);
            }

            void Descriptor(int Value)
            {
                *((int32_t *)&Data) = Value;
            }

            _FORCE_INLINE bool Happened(uint32_t Masks)
            {
                return (Events & Masks) != 0;
            }

            _FORCE_INLINE void Set(uint32_t Masks)
            {
                Events |= Masks;
            }

            _FORCE_INLINE void Reset(uint32_t Masks)
            {
                Events &= ~Masks;
            }
        };

        class ePoll : private Descriptor
        {
        public:

        private:
        public:
            // ### Types

            enum Commands
            {
                Add = EPOLL_CTL_ADD,
                Remove = EPOLL_CTL_DEL,
                Modify = EPOLL_CTL_MOD,
            };

            enum ePollEvents
            {
                In = EPOLLIN,
                Out = EPOLLOUT,
                HangUp = EPOLLHUP,
                Error = EPOLLERR,
            };

            ePoll(int Flags = 0)
            {
                _INode = epoll_create1(Flags);
            }
            ~ePoll() {}

            void Set(CEPoll &Item, uint32_t Masks)
            {
                Item.Set(Masks);
                epoll_ctl(_INode, Modify, Item.Descriptor(), (epoll_event *)&Item);
            }

            void Reset(CEPoll &Item, uint32_t Masks)
            {
                Item.Reset(Masks);
                epoll_ctl(_INode, Modify, Item.Descriptor(), (epoll_event *)&Item);
            }
        };
    }
}