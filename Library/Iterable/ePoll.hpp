#pragma once

#include <sys/epoll.h>

#include "Iterable/List.hpp"
#include "Descriptor.hpp"

namespace Core
{
    namespace Iterable
    {
        class ePoll : private Descriptor
        {
        public:
            struct Container
            {
                uint32_t Events;
                uint64_t Data;

                int Descriptor() const
                {
                    return *((int32_t *)&Data);
                }

                void Descriptor(int Value)
                {
                    *((int32_t *)&Data) = Value;
                }

                template<typename TReturn>
                TReturn& DataAs()
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

                static Container From(int Descriptor, uint32_t Events)
                {
                    Container Result;
                    Result.Descriptor(Descriptor);
                    Result.Events = Events;
                    return Result;
                }

                static Container From(uint64_t Data, uint32_t Events)
                {
                    Container Result;
                    Result.Data = Data;
                    Result.Events = Events;
                    return Result;
                }
            } __EPOLL_PACKED;

        private:
        public:
            // ### Types

            enum class Commands : int
            {
                Add = EPOLL_CTL_ADD,
                Modify = EPOLL_CTL_MOD,
                Delete = EPOLL_CTL_DEL,
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

            ~ePoll()
            {
                close(_INode);
            }

            void Add(const Descriptor &descriptor, uint32_t Events, uint64_t Data)
            {
                Container _Container = Container::From(Data, Events);

                if (epoll_ctl(_INode, int(Commands::Add), descriptor.INode(), (struct epoll_event *)&_Container) == -1)
                {
                    throw std::system_error(errno, std::generic_category());
                }
            }

            void Add(const Descriptor &descriptor, uint32_t Events)
            {
                Container _Container = Container::From(descriptor.INode(), Events);

                if (epoll_ctl(_INode, int(Commands::Add), descriptor.INode(), (struct epoll_event *)&_Container) == -1)
                {
                    throw std::system_error(errno, std::generic_category());
                }
            }

            void Delete(const Descriptor &descriptor)
            {
                if (epoll_ctl(_INode, int(Commands::Delete), descriptor.INode(), (struct epoll_event *) nullptr) == -1)
                {
                    throw std::system_error(errno, std::generic_category());
                }
            }

            void Modify(const Descriptor &descriptor, uint32_t Events)
            {
                Container _Container = Container::From(descriptor.INode(), Events);

                epoll_ctl(_INode, int(Commands::Modify), descriptor.INode(), (struct epoll_event *)&_Container);
            }

            void operator()(Core::Iterable::List<Container> &Items, int Timeout = -1)
            {
                int Count = epoll_wait(_INode, (struct epoll_event *) Items.Content(), Items.Capacity(), Timeout);

                if (Count == -1)
                {
                    throw std::system_error(errno, std::generic_category());
                }

                Items.Length(Count);
            }
        };
    }
}