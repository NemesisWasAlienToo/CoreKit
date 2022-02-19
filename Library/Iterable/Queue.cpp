#pragma once

#ifndef _FORCE_INLINE
#define _FORCE_INLINE __attribute__((always_inline))
#endif

#include <iostream>
#include <sstream>
#include <functional>
#include <string>
#include <cstring>

#include "Iterable/Span.cpp"

namespace Core
{
    namespace Iterable
    {
        template <typename T>
        class Queue : public Iterable<T>
        {

        protected:
            // ### Private variables

            size_t _First = 0;

            // ### Private Functions

            _FORCE_INLINE inline T &_ElementAt(size_t Index) override
            {
                return this->_Content[(_First + Index) % this->_Capacity];
            }

            _FORCE_INLINE inline const T &_ElementAt(size_t Index) const override
            {
                return this->_Content[(_First + Index) % this->_Capacity];
            }

        public:
            // ### Constructors

            Queue() = default;

            Queue(size_t Capacity, bool Growable = true) : Iterable<T>(Capacity, Growable), _First(0) {}

            Queue(T *Array, int Count, bool Growable = true) : Iterable<T>(Array, Count, Growable), _First(0) {}

            Queue(const Queue &Other) : Iterable<T>(Other), _First(0) {}

            Queue(Queue &&Other) noexcept : Iterable<T>(std::move(Other)), _First(Other._First)
            {
                Other._First = 0;
            }

            // ### Destructor

            ~Queue() = default;

            // ### Properties

            std::tuple<const T*, size_t> Chunk()
            {
                return std::tuple(&_ElementAt(0), std::min((this->_Capacity - this->_Length), this->_Length));
            }

            // ### Public Functions

            void Resize(size_t Size) // @todo Fix this bug
            {
                Iterable<T>::Resize(Size);

                _First = 0;
            }

            T Take()
            {
                if (this->IsEmpty())
                    throw std::out_of_range("");

                T Item = std::move(_ElementAt(0)); // OK?
                this->_Length--;
                _First = (_First + 1) % this->_Capacity;

                return Item;
            }

            void Take(T *Items, size_t Count)
            {
                if (this->_Length < Count)
                    throw std::out_of_range("");

                for (size_t i = 0; i < Count; i++)
                {
                    Items[i] = std::move(_ElementAt(i));
                }

                _First = (_First + Count) % this->_Capacity;
                this->_Length -= Count;
            }

            void Free()
            {
                if constexpr (!std::is_arithmetic<T>::value)
                {
                    for (size_t i = 0; i < this->_Length; i++)
                    {
                        _ElementAt(i).~T();
                    }
                }

                this->_First = 0;
                this->_Length = 0;
            }

            void Free(size_t Count)
            {
                if (this->_Length < Count)
                    throw std::out_of_range("");

                if constexpr (!std::is_arithmetic<T>::value)
                {
                    for (size_t i = 0; i < Count; i++)
                    {
                        _ElementAt(i).~T();
                    }
                }

                _First = (_First + Count) % this->_Capacity;
                this->_Length -= Count;
            }

            std::string ToString(size_t Size)
            {
                if (Size > this->_Length)
                    throw std::out_of_range("");

                // @todo Cahange to steam and display object

                std::string str;

                str.resize(Size * sizeof(T));

                for (size_t i = 0; i < Size; i++)
                {
                    str[i] = _ElementAt(i);
                }

                return str;
            }

            std::string ToString()
            {
                return ToString(this->_Length);
            }

            // ### Operators

            T &operator[](const size_t &Index)
            {
                if (Index >= this->_Length)
                    throw std::out_of_range("");

                return _ElementAt(Index);
            }

            Queue &operator=(const Queue &Other)
            {
                this->_Capacity = Other._Capacity;
                this->_Length = Other._Length;
                _First = 0;

                delete[] this->_Content;

                this->_Content = new T[Other._Capacity];

                for (size_t i = 0; i < Other._Length; i++)
                {
                    _ElementAt(i) = Other._ElementAt(i);
                }

                return *this;
            }

            Queue &operator=(Queue &&Other) noexcept
            {
                std::swap(this->_Content, Other._Content);
                std::swap(this->_Capacity, Other._Capacity);
                std::swap(this->_Length, Other._Length);
                std::swap(this->_Growable, Other._Growable);
                std::swap(_First, Other._First);

                return *this;
            }

            Queue &operator>>(T &Item)
            {

                Item = Take();

                return *this;
            }

            friend std::ostream &operator<<(std::ostream &os, Queue &Queue)
            {
                while (!Queue.IsEmpty())
                {
                    os << Queue.Take();
                }

                return os;
            }
        };
    }
}