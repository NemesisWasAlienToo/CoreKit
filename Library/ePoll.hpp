#pragma once

#include <sys/epoll.h>

#include "Iterable/List.hpp"
#include "Descriptor.hpp"

namespace Core
{
    class ePoll : public Descriptor
    {
    public:
        struct Entry
        {
            uint32_t Events;
            uint64_t Data;

            template <typename TReturn>
            inline TReturn &DataAs()
            {
                return *(reinterpret_cast<TReturn *>(&Data));
            }

            _FORCE_INLINE bool Happened(uint32_t Masks) const
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

            static Entry From(const Descriptor &descriptor, uint32_t Events)
            {
                Entry Result;
                Result.DataAs<int>() = descriptor.INode();
                Result.Events = Events;
                return Result;
            }

            static Entry From(uint64_t Data, uint32_t Events)
            {
                Entry Result;
                Result.Data = Data;
                Result.Events = Events;
                return Result;
            }
        } __EPOLL_PACKED;

        using List = Iterable::List<Entry>;

    private:
    public:
        // ### Types

        enum class Commands : int
        {
            Add = EPOLL_CTL_ADD,
            Modify = EPOLL_CTL_MOD,
            Delete = EPOLL_CTL_DEL,
        };

        enum ePollOptions
        {
            In = EPOLLIN,
            UrgentIn = POLLPRI,
            Out = EPOLLOUT,
            Error = EPOLLERR,
            HangUp = EPOLLHUP,
            ReadHangUp = EPOLLRDHUP,
            EdgeTriggered = EPOLLET,
            OneShot = EPOLLONESHOT,
        };

        ePoll(int Flags = 0)
        {
            _INode = epoll_create1(Flags);
        }

        ~ePoll()
        {
            Close();
        }

        void Add(const Descriptor &descriptor, uint32_t Events, uint64_t Data)
        {
            Entry _Entry = Entry::From(Data, Events);

            if (epoll_ctl(_INode, int(Commands::Add), descriptor.INode(), (struct epoll_event *)&_Entry) == -1)
            {
                throw std::system_error(errno, std::generic_category());
            }
        }

        void Add(const Descriptor &descriptor, uint32_t Events)
        {
            Entry _Entry = Entry::From(descriptor.INode(), Events);

            if (epoll_ctl(_INode, int(Commands::Add), descriptor.INode(), (struct epoll_event *)&_Entry) == -1)
            {
                throw std::system_error(errno, std::generic_category());
            }
        }

        void Modify(const Descriptor &descriptor, uint32_t Events, uint64_t Data)
        {
            Entry _Entry = Entry::From(Data, Events);

            epoll_ctl(_INode, int(Commands::Modify), descriptor.INode(), (struct epoll_event *)&_Entry);
        }

        void Modify(const Descriptor &descriptor, uint32_t Events)
        {
            Entry _Entry = Entry::From(descriptor.INode(), Events);

            epoll_ctl(_INode, int(Commands::Modify), descriptor.INode(), (struct epoll_event *)&_Entry);
        }

        void Delete(const Descriptor &descriptor)
        {
            if (epoll_ctl(_INode, int(Commands::Delete), descriptor.INode(), (struct epoll_event *)nullptr) == -1)
            {
                throw std::system_error(errno, std::generic_category());
            }
        }

        void operator()(List &Items, int Timeout = -1)
        {
            int Count = epoll_wait(_INode, (struct epoll_event *)Items.Content(), static_cast<int>(Items.Capacity()), Timeout);
            int Saved = errno;

            if (Count == -1 && Saved != EINTR)
            {
                throw std::system_error(Saved, std::generic_category());
            }

            Items.Length(Count);
        }
    };
}