#pragma once

#include <poll.h>

#include "Iterable/List.cpp"
#include "Network/Socket.cpp"
#include "Base/Descriptor.cpp"

namespace Base
{
    template <typename T>
    class Poll
    {
        static_assert(std::is_base_of<Base::Descriptor, T>::value, "T must inherit from Base::Descriptor");

    private:
        // Types :

        typedef struct pollfd _POLLFD;
        typedef std::function<void(T &, int)> Callback;

        // Variables :

        _POLLFD *_Content = NULL;
        int _Capacity = 0;
        int _Length = 0;

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

        Poll(int Capacity) : _Content(new _POLLFD[Capacity]), _Capacity(Capacity), _Length(0) {}

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

        void Add(T &Descriptor, Event Events)
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

        void Remove(T &Descriptor)
        {
            int Handler = Descriptor.Handler();

            if (Handler < 0)
            {
                std::cout << "Error : Invalid file descriptor (less than zero)" << std::endl;
                exit(-1);
            }

            for (int i = 0; i < _Length; i++)
            {
                if (_Content[i].fd == Handler)
                {
                    if (i == _Length)
                    {
                    }
                    _Content[i] = _Content[--_Length];
                    _Content[_Length].events = 0;
                    break;
                }
            }
        }

        void Remove(int Index)
        {
            auto &item = _Content[Index];
            auto &last = _Content[--_Length];
            item = last;
            last.fd = 0;
            last.events = 0;
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

            for (int i = 0, j = 0; i < Result && j < _Length; j++)
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
        }
    };
}
