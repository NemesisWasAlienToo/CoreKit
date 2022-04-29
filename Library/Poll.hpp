#pragma once

#include <poll.h>
#include <system_error>

#include "Iterable/Iterable.hpp"
#include "Descriptor.hpp"

namespace Core
{

    class Poll
    {
    public:
        struct Container
        {
            int Descriptor;
            short int Mask;
            short int Events;

            Container() = default;

            constexpr Container(const Core::Descriptor &Descriptor, short int Masks) noexcept
            {
                this->Descriptor = Descriptor.INode();
                this->Mask = Masks;
                this->Events = 0;
            }

            template <typename TReturn>
            inline TReturn &DescriptorAs()
            {
                return *reinterpret_cast<TReturn *>(&Descriptor);
            }

            _FORCE_INLINE inline bool HasEvent()
            {
                return bool(Events);
            }

            _FORCE_INLINE inline bool Happened(short int Masks)
            {
                return bool(Events & Masks);
            }

            _FORCE_INLINE inline void Set(short int Masks)
            {
                Mask |= Masks;
            }

            _FORCE_INLINE inline void Reset(short int Masks)
            {
                Mask &= ~Masks;
            }
        };

        typedef short Event;

        enum PollEvents
        {
            In = POLLIN,
            Out = POLLOUT,
            Error = POLLNVAL,
        };

        Poll() = default;

        // Operators

        int operator()(Iterable::Iterable<Container> &Events, int Timeout = -1)
        {
            int Result = poll((pollfd *)Events.Content(), Events.Length(), Timeout);

            if (Result == -1)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return Result;
        }

        Poll &operator=(const Poll &Other) = delete;

        Poll &operator=(Poll &&Other) noexcept = default;
    };
}
