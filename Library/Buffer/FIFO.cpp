#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <cstring>

namespace Core
{
    namespace Buffer
    {
        class FIFO
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
            FIFO() = default;

            FIFO(size_t Capacity) : _Capacity(Capacity), _Content(new char[Capacity]), _First(_Content) {}

            FIFO(FIFO &Other) : _Capacity(Other._Capacity), _Content(new char[Other._Capacity]), _First(Other._First), _Last(Other._Last)
            {

                for (size_t i = 0; i < Other._Capacity; i++)
                {
                    _Content[i] = char(_Content[i]);
                }
            }

            FIFO(FIFO &&Other)
            noexcept : _Capacity(Other._Capacity), _First(Other._First), _Last(Other._Last)
            {
                std::swap(_Content, Other._Content);
            }

            ~FIFO()
            {
                delete[] _Content;
            }

            char * Data()
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

            void Resize(size_t Size)
            {
                size_t len = Length();
                char * _New = new char[Size];

                for (size_t i = 0; i < len; i++)
                {
                    int j = Wrap(i);
                    _New[j] = char(_Content[j]);
                }
                
                delete[] _Content;

                _Content = _New;
            }

            bool IsEmpty() { return _Last == NULL; }

            bool IsFull() { return _End == NULL ? true : (_First == &_Last[1]) || (_First == _Start && _Last == _End); }

            void Empty()
            {
                _First = _Start;
                _Last = NULL;
            }

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

            size_t Free(size_t Count)
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

            void Peak(char * Buffer, size_t Size = 0){
                if(Size > Length()) throw std::out_of_range("");

                if(Size == 0) Size = Length();

                for (size_t i = 0; i < Size ; i++)
                {
                    Buffer[i] = _Content[Wrap(i)];
                }
            }

            std::string Peak(size_t Size = 0){
                if(Size > Length()) throw std::out_of_range("");

                if(Size == 0) Size = Length();

                std::stringstream ss;

                for (size_t i = 0; i < Size; i++)
                {
                    ss << _Content[Wrap(i)];
                }

                return ss.str();
            }

            FIFO &operator=(FIFO &Other) = delete;

            FIFO &operator=(FIFO &&Other) noexcept
            {
                if (this == &Other)
                    return *this;

                delete[] _Content;

                _Capacity = Other._Capacity;
                _First = Other._First;
                _Last = Other._Last;

                std::swap(_Content, Other._Content);

                return *this;
            }

            char &operator[](const size_t &index)
            {
                if(index > Length()) throw std::out_of_range("");
                return _Content[Wrap(index)];
            }

            FIFO &operator>>(std::string &str)
            {
                char Item;

                while (!IsEmpty())
                {
                    Take(Item);
                    str += (Item);
                }

                return *this;
            }

            // ## Handle termination character
            FIFO &operator>>(char &Item)
            {

                if (!IsEmpty())
                    Take(Item);
                return *this;
            }

            // ## Handle termination character
            FIFO &operator<<(const char &Item)
            {
                if (!IsFull())
                    Put(Item);
                return *this;
            }

            FIFO &operator<<(const char *Item)
            {
                for (size_t i = 0; !IsFull() && Item[i] != 0; i++)
                {
                    Put(Item[i]);
                }
                return *this;
            }

            friend std::ostream &operator<<(std::ostream &os, FIFO &buffer)
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
}