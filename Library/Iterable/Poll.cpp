#pragma once

#include <poll.h>
#include <system_error>

#include "Iterable/Iterable.cpp"
#include "Network/Socket.cpp"
#include "Descriptor.cpp"

namespace Core
{
    namespace Iterable
    {
        struct CPoll
        {
            int Descriptor = -1;
            short int Mask = 0;
            short int Events = 0;

            _FORCE_INLINE inline bool HasEvent()
            {
                return Events != 0;
            }

            _FORCE_INLINE inline bool Happened(short int Masks)
            {
                return (Events & Masks) != 0;
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

        class Poll : public Iterable<CPoll>
        {
        public:
            typedef short Event;

            enum PollEvents
            {
                In = POLLIN,
                Out = POLLOUT,
                Error = POLLNVAL,
            };

            Poll() = default;

            Poll(size_t Capacity) : Iterable<CPoll>(Capacity) {}
            Poll(Poll &Other) : Iterable<CPoll>(Other) {}
            Poll(Poll &&Other) noexcept : Iterable<CPoll>(std::move(Other)) {}

            void Resize(size_t Size) {
                this->_Content = (CPoll *)std::realloc(this->_Content, Size);
            }
            
            void Add(const Descriptor &Descriptor, Event Events)
            {
                int Handler = Descriptor.INode();

                if (Handler < 0)
                {
                    std::cout << "Error : Invalid file descriptor (less than zero)" << std::endl;
                    exit(-1);
                }

                _IncreaseCapacity();

                CPoll &Poll = _Content[_Length++];
                Poll.Descriptor = Descriptor.INode();
                Poll.Mask = Events;
            }

            void Add(CPoll &&Item)
            {
                this->_IncreaseCapacity();

                _ElementAt(this->_Length) = Item;
                (this->_Length)++;
            }

            void Add(const CPoll &Item)
            {
                this->_IncreaseCapacity();

                _ElementAt(this->_Length) = Item;
                (this->_Length)++;
            }

            // ### We dont need this
            void Add(const CPoll &Item, size_t Count)
            {
                this->_IncreaseCapacity(Count);

                for (size_t i = 0; i < Count; i++)
                {
                    _ElementAt(this->_Length + i) = Item;
                }

                this->_Length += Count;
            }

            void Add(CPoll *Items, size_t Count)
            {
                this->_IncreaseCapacity(Count);

                for (size_t i = 0; i < Count; i++)
                {
                    _ElementAt(this->_Length + i) = Items[i];
                }

                this->_Length += Count;
            }

            void Add(const CPoll *Items, size_t Count)
            {
                this->_IncreaseCapacity(Count);

                for (size_t i = 0; i < Count; i++)
                {
                    _ElementAt(this->_Length + i) = Items[i];
                }

                this->_Length += Count;
            }

            void Fill(const CPoll &Item)
            {
                for (size_t i = this->_Length; i < this->_Capacity; i++)
                {
                    _ElementAt(i) = Item;
                }

                this->_Length = this->_Capacity;
            }

            CPoll Take()
            {
                if (this->_Length == 0)
                    throw std::out_of_range("");

                this->_Length--;

                return _ElementAt(this->_Length);
            }

            void Take(CPoll *Items, size_t Count)
            {
                if (this->_Length < Count)
                    throw std::out_of_range("");

                size_t _Length_ = this->_Length - Count;

                for (size_t i = _Length_; i < this->_Length; i++)
                {
                    Items[i] = _ElementAt(i);
                }

                this->_Length = _Length_;
            }

            void Remove(size_t Index)
            {
                if (Index >= _Length)
                    throw std::out_of_range("");

                _Length--;

                if (Index == _Length)
                    return;

                for (size_t i = Index; i < _Length; i++)
                {
                    _ElementAt(i) = _ElementAt(i + 1);
                }
            }

            void Swap(size_t Index)
            {
                if (Index >= _Length)
                    throw std::out_of_range("");

                if (_Length - 1 == Index)
                {
                    --_Length;
                }
                else
                {
                    _ElementAt(Index) = std::move(_ElementAt(--_Length));
                }
            }

            void Swap(size_t First, size_t Second)
            {
                if (First >= _Length || Second >= _Length)
                    throw std::out_of_range("");

                std::swap(_ElementAt(First), _ElementAt(Second));
            }

            // ### Additional functionalities

            void ForEach(const std::function<void(Descriptor)>& Action)
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    Action(_ElementAt(i).Descriptor);
                }
            }

            void ForEach(const std::function<void(size_t, Descriptor)>& Action)
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    Action(i, _ElementAt(i).Descriptor);
                }
            }

            

            // Operators

            int operator()(int TimeoutMS = -1)
            {
                int Result = poll((pollfd *)_Content, _Length, TimeoutMS);

                if (Result == -1)
                {
                    throw std::system_error(errno, std::generic_category());
                }
                
                return Result;
            }

            CPoll &operator[](size_t Index)
            {
                return _Content[Index];
            }

            Descriptor operator()(size_t Index)
            {
                return _Content[Index].Descriptor;
            }

            Poll &operator=(const Poll &Other) = delete;

            Poll &operator=(Poll &&Other) noexcept = default;
        };
    }
}
