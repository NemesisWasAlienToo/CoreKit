#pragma once

#include <poll.h>

#include "Iterable/List.cpp"
#include "Network/Socket.cpp"
#include "Base/Descriptor.cpp"

namespace Core
{
    namespace Iterable
    {
        template <typename T = Descriptor>
        class Poll
        {
            static_assert(std::is_base_of<Descriptor, T>::value, "T must inherit from Descriptor");

        private:
            // Types :

            typedef struct pollfd _POLLFD;
            typedef std::function<void(const T &, size_t)> Callback;

            // Variables :

            _POLLFD *_Content = NULL;
            size_t _Capacity = 0;
            size_t _Length = 0;

            // Functions :

            void increaseCapacity()
            {
                if (_Capacity > _Length)
                    return;
                if (_Capacity > 0)
                    _Content = (_POLLFD *)std::realloc(_Content, sizeof(_POLLFD) * ++_Capacity);
                if (_Capacity == 0)
                    _Content = new _POLLFD[++_Capacity];
            }

            _FORCE_INLINE inline _POLLFD &_ElementAt(size_t Index)
            {
                return _Content[Index];
            }

            _FORCE_INLINE inline const _POLLFD &_ElementAt(size_t Index) const
            {
                return _Content[Index];
            }

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

            Poll(size_t Capacity) : _Content(new _POLLFD[Capacity]), _Capacity(Capacity), _Length(0) {}

            Poll(Poll &Other) : _Content(new _POLLFD[Other._Capacity]), _Capacity(Other._Capacity), _Length(Other._Length), OnRead(Other.OnRead), OnWrite(Other.OnWrite), OnError(Other.OnError)
            {
                for (size_t i = 0; i < Other._Length; i++)
                {
                    _Content[i] = _POLLFD(Other._Content[i]);
                }
            }

            Poll(Poll &&Other) noexcept : _Content(new _POLLFD[Other._Capacity]), _Capacity(Other._Capacity), _Length(Other._Length), OnRead(Other.OnRead), OnWrite(Other.OnWrite), OnError(Other.OnError)
            {
                std::swap(_Content, Other._Content);
            }

            // Poll(Iterable::List<T> Descriptors){}

            ~Poll()
            {
                delete[] _Content;
            }

            void Add(const T &Descriptor, Event Events)
            {
                int Handler = Descriptor.Handler();

                if (Handler < 0)
                {
                    std::cout << "Error : Invalid file descriptor (less than zero)" << std::endl;
                    exit(-1);
                }

                increaseCapacity();
                _POLLFD &POLL = _Content[_Length++];
                POLL.fd = Descriptor.Handler();
                POLL.events = Events;
            }

            // void Remove(T &Descriptor)
            // {
            //     int Handler = Descriptor.Handler();

            //     if (Handler < 0)
            //     {
            //         std::cout << "Error : Invalid file descriptor (less than zero)" << std::endl;
            //         exit(-1);
            //     }

            //     for (int i = 0; i < _Length; i++)
            //     {
            //         if (_Content[i].fd == Handler)
            //         {
            //             if (i == _Length)
            //             {
            //             }
            //             _Content[i] = _Content[--_Length];
            //             _Content[_Length].events = 0;
            //             break;
            //         }
            //     }
            // }

            // void Remove(int Index)
            // {
            //     auto &item = _Content[Index];
            //     auto &last = _Content[--_Length];
            //     item = last;
            //     last.fd = 0;
            //     last.events = 0;
            // }

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
                    return;

                _ElementAt(Index) = _ElementAt(--_Length);
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
                        T Descriptor(item.fd);

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

            void ForEach(std::function<void(const Descriptor &)> Action) const
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    Action(_ElementAt(i));
                }
            }

            void ForEach(std::function<void(int, const Descriptor &)> Action) const
            {
                for (int i = 0; i < _Length; i++)
                {
                    Action(i, _ElementAt(i));
                }
            }

            T &operator[](int Index)
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

                _Content = Other._Capacity;
                _Capacity = Other._Capacity;
                _Length = Other._Length;

                OnRead = Other.OnRead;
                OnWrite = Other.OnWrite;
                OnError = Other.OnError;

                std::swap(_Content, Other._Content);

                return *this;
            }
        };

        template <>
        class Poll<Descriptor>
        {
        public:
            typedef short Event;

            enum PollEvents
            {
                In = POLLIN,
                Out = POLLOUT,
                Error = POLLNVAL,
            };
        };
    }
}
