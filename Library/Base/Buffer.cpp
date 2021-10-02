#pragma once

#include <iostream>
#include <string>
#include <cstring>

namespace Base
{
    class Buffer
    {
    private:
        size_t _Capacity = 0;
        char *_Content = NULL;
        char *_First = NULL;
        char *_Last = NULL;

#define _Start (_Content)
#define _End (_Capacity > 0 ? &_Content[_Capacity - 1] : NULL)

#define Index(Pointer) (Pointer - _Start)
#define Min(First, Second) (First <= Second ? First : Second)
#define Max(First, Second) (First >= Second ? First : Second)
#define Wrap(Indx) ((Index(_First) + Indx) % _Capacity)

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

        char *Data()
        {
            return _First;
        }

        size_t Length()
        {
            return _Last == NULL ? 0 : _First < _Last ? (_Last - _First + 1)
                                                      : ((_Last - _Start + 1) + (_First - _End + 1));
        }

        size_t Capacity()
        {
            return _Capacity;
        }

        bool IsEmpty() { return _Last == NULL; }

        // Needs test
        bool IsFull() { return _End == NULL ? true : (_First == &_Last[1]) || (_First == _Start && _Last == _End); }

        bool Put(const char &Item) // Call constructor
        {
            if (IsFull())
                return false;

            if (IsEmpty())
            {
                *_First = char(Item); // Invoke copy operator of char
                _Last = _First;
            }
            else
            {
                *(++_Last) = char(Item);
            }

            return true;
        }

        bool Take(char &Item) // Call dispose ?
        {
            if (IsEmpty())
                return false;

            if (_First == _Last)
            {
                _Last = NULL;
                Item = char(*(_First));
                return true;
            }
            else
            {
                Item = char(*(_First++));

                return true;
            }
        }

        size_t Skip(size_t Count)
        {

            size_t _Count = 0, Len = Length();

            if (Count >= Len)
            {
                _Count = Len;
                _Last = NULL;
            }
            else
            {
                _Count = Count;
                _First = _Start + Wrap(Count);
            }

            return _Count;
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

        friend std::ostream &operator<<(std::ostream &os, Buffer &buffer)
        {
            char Item;

            while (!buffer.IsEmpty())
            {
                buffer.Take(Item);
                os << Item;
            }

            return os;
        }
    };
}