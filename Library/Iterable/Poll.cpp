#pragma once

#include <poll.h>

#include "Iterable/Iterable.cpp"
#include "Network/Socket.cpp"
#include "Base/Descriptor.cpp"

namespace Core
{
    namespace Iterable
    {
        struct CPoll
        {
            int Descriptor;
            short int Mask;
            short int Events;

            _FORCE_INLINE bool Happened(short int Masks)
            {
                return (Events & Masks) != 0;
            }

            _FORCE_INLINE void Listen(short int Masks)
            {
                Mask |= Masks;
            }

            _FORCE_INLINE void Ignore(short int Masks)
            {
                Mask &= ~Masks;
            }
        };

        class Poll : public Iterable<CPoll>
        {
        private:
            // Types :

            typedef std::function<void(Descriptor, size_t)> Callback;

        public:
            typedef short Event;

            enum PollEvents
            {
                In = POLLIN,
                Out = POLLOUT,
                Error = POLLNVAL,
            };

            Callback OnRead = NULL;
            Callback OnWrite = NULL;
            Callback OnError = NULL;

            Poll() = default;

            Poll(size_t Capacity) : Iterable<CPoll>(Capacity) {}
            Poll(Poll &Other) : Iterable<CPoll>(Other), OnRead(Other.OnRead), OnWrite(Other.OnWrite), OnError(Other.OnError) {}
            Poll(Poll &&Other) noexcept : Iterable<CPoll>(std::move(Other)), OnRead(Other.OnRead), OnWrite(Other.OnWrite), OnError(Other.OnError) {}

            void Resize(size_t Size) override {
                this->_Content = (CPoll *)std::realloc(this->_Content, Size);
            }
            
            void Add(const Descriptor &Descriptor, Event Events)
            {
                int Handler = Descriptor.Handler();

                if (Handler < 0)
                {
                    std::cout << "Error : Invalid file descriptor (less than zero)" << std::endl;
                    exit(-1);
                }

                _IncreaseCapacity();

                CPoll &Poll = _Content[_Length++];
                Poll.Descriptor = Descriptor.Handler();
                Poll.Mask = Events;
            }

            void Add(CPoll &&Item) override
            {
                this->_IncreaseCapacity();

                _ElementAt(this->_Length) = std::move(Item);
                (this->_Length)++;
            }

            void Add(const CPoll &Item) override
            {
                this->_IncreaseCapacity();

                _ElementAt(this->_Length) = Item;
                (this->_Length)++;
            }

            void Add(const CPoll &Item, size_t Count) override
            {
                this->_IncreaseCapacity(Count);

                for (size_t i = 0; i < Count; i++)
                {
                    _ElementAt(this->_Length + i) = Item;
                }

                this->_Length += Count;
            }

            void Remove(size_t Index) // Not Compatiable
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

            void Swap(size_t Index) // Not Compatiable
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

            void Swap(size_t First, size_t Second) // Not Compatiable
            {
                if (First >= _Length || Second >= _Length)
                    throw std::out_of_range("");

                std::swap(_ElementAt(First), _ElementAt(Second));
            }

            void Await(int TimeoutMS = -1)
            {
                int Result;

                Result = poll((pollfd *)_Content, _Length, TimeoutMS);

                // Error handling here

                if (Result == -1)
                {
                    std::cout << "Error :" << strerror(errno) << std::endl;
                    exit(-1);
                }

                if (Result == 0)
                    return;

                int i = 0;
                size_t j = 0;

                for (; i < Result && j < _Length; j++)
                {
                    auto &item = _Content[j];

                    if (item.Events)
                    {
                        i++;
                        Descriptor Descriptor(item.Descriptor);

                        if ((this->OnRead != NULL) && item.Happened(In))
                        {
                            this->OnRead(Descriptor, j);
                        }

                        if ((this->OnWrite != NULL) && item.Happened(Out))
                        {
                            this->OnWrite(Descriptor, j);
                        }

                        if ((this->OnError != NULL) && item.Happened(Error))
                        {
                            this->OnError(Descriptor, j);
                        }
                    }
                }
            }

            void ForEach(std::function<void(Descriptor)> Action)
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    Action(_ElementAt(i).Descriptor);
                }
            }

            void ForEach(std::function<void(size_t, Descriptor)> Action)
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    Action(i, _ElementAt(i).Descriptor);
                }
            }

            CPoll &operator[](size_t Index)
            {
                return _Content[Index];
            }

            Descriptor operator()(size_t Index)
            {
                return _Content[Index].Descriptor;
            }

            // void operator()(int TimeoutMS = -1)
            // {
            //     int Result;

            //     Result = poll((pollfd *)_Content, _Length, TimeoutMS);

            //     // Error handling here

            //     if (Result == -1)
            //     {
            //         std::cout << "Error :" << strerror(errno) << std::endl;
            //         exit(-1);
            //     }

            //     if (Result == 0)
            //         return;

            //     int i = 0;
            //     size_t j = 0;

            //     for (; i < Result && j < _Length; j++)
            //     {
            //         auto &item = _Content[j];

            //         if (item.Events)
            //         {
            //             i++;
            //             Descriptor Descriptor(item.Descriptor);

            //             if ((this->OnRead != NULL) && item.Happened(In))
            //             {
            //                 this->OnRead(Descriptor, j);
            //             }

            //             if ((this->OnWrite != NULL) && item.Happened(Out))
            //             {
            //                 this->OnWrite(Descriptor, j);
            //             }

            //             if ((this->OnError != NULL) && item.Happened(Error))
            //             {
            //                 this->OnError(Descriptor, j);
            //             }
            //         }
            //     }
            // }

            Poll &operator=(Poll &Other) = delete;

            // Implement later
            Poll &operator=(Poll &&Other) noexcept
            {
                if (this == &Other)
                    return *this;

                delete[] _Content;

                _Content = Other._Content;
                _Capacity = Other._Capacity;
                _Length = Other._Length;

                OnRead = Other.OnRead;
                OnWrite = Other.OnWrite;
                OnError = Other.OnError;

                std::swap(_Content, Other._Content);

                return *this;
            }
        };
    }
}
