#pragma once

#include <sys/epoll.h>

#include "Iterable/List.hpp"
#include "Descriptor.hpp"

namespace Core
{
    class ePoll : public Descriptor
    {
    public:
        using Event = uint32_t;
        struct Entry
        {
            uint32_t Events;
            uint64_t Data;

            inline bool Happened(uint32_t Masks) const
            {
                return (Events & Masks) != 0;
            }

            inline void Set(uint32_t Masks)
            {
                Events |= Masks;
            }

            inline void Reset(uint32_t Masks)
            {
                Events &= ~Masks;
            }

            static Entry From(const Descriptor &descriptor, uint32_t Events)
            {
                Entry Result;
                Result.Data = static_cast<uint64_t>(descriptor.INode());
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

        ePoll() = default;

        ePoll(ePoll const &) = delete;
        ePoll(ePoll &&Other) noexcept : Descriptor(std::move(Other)) {}

        ePoll(int Flags)
        {
            _INode = epoll_create1(Flags);
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
            Entry _Entry = Entry::From(static_cast<uint64_t>(descriptor.INode()), Events);

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

        void Modify(const Descriptor &descriptor, uint32_t Events) const
        {
            Entry _Entry = Entry::From(static_cast<uint64_t>(descriptor.INode()), Events);

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
            int Count = 0;
            int Saved = 0;

            do
            {
                Count = epoll_wait(_INode, (struct epoll_event *)Items.Content(), static_cast<int>(Items.Capacity()), Timeout);
                Saved = errno;
            } while (Count < 0 && Saved == EINTR);

            if (Count == -1)
            {
                throw std::system_error(Saved, std::generic_category());
            }

            Items.Length(static_cast<size_t>(Count));
        }

        ePoll &operator=(ePoll const &Other) = delete;

        ePoll &operator=(ePoll &&Other) noexcept
        {
            Descriptor::operator=(std::move(Other));

            return *this;
        }
    };
}