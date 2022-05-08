#pragma once

#ifndef _FORCE_INLINE
#define _FORCE_INLINE __attribute__((always_inline))
#endif

#include <iostream>
#include <sstream>
#include <functional>
#include <string>
#include <cstring>

#include "Iterable/Span.hpp"

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
                return std::tuple(&_ElementAt(0), std::min((this->_Capacity - this->_First), this->_Length));
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

            // ### Operators

            Queue &operator=(const Queue &Other) = default;

            Queue &operator=(Queue &&Other) noexcept = default;

            T &operator[](const size_t &Index)
            {
                if (Index >= this->_Length)
                    throw std::out_of_range("");

                return _ElementAt(Index);
            }

            Queue &operator>>(T &Item)
            {

                Item = Take();

                return *this;
            }
        };
    }
}