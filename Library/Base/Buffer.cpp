#pragma once

#include <iostream>
#include <string>
#include <cstring>
namespace Core
{
    class Buffer
    {
    private:
        size_t _Capacity = 0;
        char *_Content = NULL;
        char *_First = NULL;
        char *_Last = NULL;

        inline char *_Start()
        {
            return _Content;
        }

        inline char *_End()
        {
            return _Capacity > 0 ? &_Content[_Capacity - 1] : NULL;
        }

        //

        inline size_t Index(const char *Pointer)
        {
            return (size_t)(Pointer - _Start());
        }

        inline size_t Wrap(size_t Indx)
        {
            return (size_t)((Index(_First) + Indx) % _Capacity);
        }

    public:
        Buffer() = default;

        Buffer(size_t Capacity) : _Capacity(Capacity), _Content(new char[Capacity]), _First(_Content) {}

        Buffer(Buffer &Other) : _Capacity(Other._Capacity), _Content(new char[Other._Capacity]), _First(Other._First), _Last(Other._Last)
        {

            for (size_t i = 0; i < Other._Capacity; i++)
            {
                _Content[i] = char(_Content[i]);
            }
        }

        Buffer(Buffer &&Other) noexcept : _Capacity(Other._Capacity), _First(Other._First), _Last(Other._Last)
        {
            std::swap(_Content, Other._Content);
        }

        ~Buffer()
        {
            delete[] _Content;
        }

        virtual char *Data() = 0;

        virtual size_t Length() = 0;

        virtual bool IsEmpty() = 0;

        virtual bool IsFull() = 0;

        virtual bool Put(const char &Item) = 0; // Call move operator (in which distructor then constructor are called)

        virtual bool Take(char &Item) = 0;

        virtual size_t Skip(size_t Count) = 0;

        size_t Capacity()
        {
            return _Capacity;
        }

        Buffer &operator=(Buffer &Other) = delete;

        Buffer &operator=(Buffer &&Other) noexcept
        {
            if (this == &Other)
                return *this;

            delete[] _Content;

            _Capacity = Other._Capacity;
            _First = Other._First;
            _Last = Other._Last;

            std::swap(_Content, Other._Content);
        }

        char &operator[](const size_t &index)
        {
            return _Content[Wrap(index)];
        }

        // ## Handle termination character
        Buffer &operator>>(char &Item)
        {

            if (!IsEmpty())
                Take(Item);
            return *this;
        }

        // ## Handle termination character
        Buffer &operator<<(const char &Item)
        {
            if (!IsFull())
                Put(Item);
            return *this;
        }

        Buffer &operator<<(const char *Item)
        {
            for (size_t i = 0; !IsFull() && Item[i] != 0; i++)
            {
                Put(Item[i]);
            }
            return *this;
        }
    };
}
