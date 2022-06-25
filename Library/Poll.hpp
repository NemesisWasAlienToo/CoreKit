#pragma once

#include <poll.h>
#include <system_error>

#include "Iterable/List.hpp"
#include "Descriptor.hpp"

namespace Core
{

    class Poll
    {
    public:
        struct Entry
        {
            int Descriptor;
            short int Mask;
            short int Events;

            Entry() = default;

            constexpr Entry(const Core::Descriptor &Descriptor, short int Masks) noexcept
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

            inline bool HasEvent()
            {
                return bool(Events);
            }

            inline bool Happened(short int Masks)
            {
                return bool(Events & Masks);
            }

            inline void Set(short int Masks)
            {
                Mask |= Masks;
            }

            inline void Reset(short int Masks)
            {
                Mask &= ~Masks;
            }
        };

        using List = Iterable::List<Entry>;

        typedef short Event;

        enum PollOptions
        {
            In = POLLIN,
            UrgentIn = POLLPRI,
            Out = POLLOUT,
            Error = POLLERR,
            HangUp = POLLHUP,
            ReadHangUp = POLLRDHUP,
            Invalid = POLLNVAL,
        };

        Poll() = default;

        // Operators

        int operator()(List &Events, int Timeout = -1)
        {
            int Result = poll((pollfd *)Events.Content(), static_cast<nfds_t>(Events.Length()), Timeout);
            int Saved = errno;

            if (Result == -1 && Saved != EINTR)
            {
                throw std::system_error(Saved, std::generic_category());
            }

            return Result;
        }
    };
}
