#pragma once

#include <poll.h>

#include "Iterable/List.cpp"
#include "Network/Socket.cpp"
#include "Base/Descriptor.cpp"

namespace Core
{
    namespace Iterable
    {
        class Poll : public List<struct pollfd>
        {
        private:
            // Types :

            typedef struct pollfd _POLLFD;
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

            Poll(size_t Capacity) : List<struct pollfd>(Capacity) {}
            Poll(Poll &Other) : List<struct pollfd>(Other), OnRead(Other.OnRead), OnWrite(Other.OnWrite), OnError(Other.OnError) {}
            Poll(Poll &&Other) noexcept : List<struct pollfd>(std::move(Other)), OnRead(Other.OnRead), OnWrite(Other.OnWrite), OnError(Other.OnError) {}

            void Add(const Descriptor &Descriptor, Event Events)
            {
                int Handler = Descriptor.Handler();

                if (Handler < 0)
                {
                    std::cout << "Error : Invalid file descriptor (less than zero)" << std::endl;
                    exit(-1);
                }

                _IncreaseCapacity();

                _POLLFD &Poll = _Content[_Length++];
                Poll.fd = Descriptor.Handler();
                Poll.events = Events;

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

                Result = poll(_Content, _Length, TimeoutMS);

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

                    if (item.revents)
                    {
                        i++;
                        Descriptor Descriptor(item.fd);

                        if ((item.revents & POLLIN) && (this->OnRead != NULL))
                        {
                            this->OnRead(Descriptor, j);
                        }

                        if ((item.revents & POLLOUT) && (this->OnWrite != NULL))
                        {
                            this->OnWrite(Descriptor, j);
                        }

                        if ((item.revents & POLLNVAL) && (this->OnError != NULL))
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
                    Action(_ElementAt(i).fd);
                }
            }

            void ForEach(std::function<void(size_t, Descriptor)> Action)
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    Action(i, _ElementAt(i).fd);
                }
            }

            Descriptor operator[](size_t Index)
            {
                return _Content[Index].fd;
            }

            _POLLFD &operator()(int Index)
            {
                return _Content[Index];
            }

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
