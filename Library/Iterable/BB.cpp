#pragma once

#include <iostream>
#include <sstream>
#include <functional>
#include <string>
#include <cstring>

#include "Iterable/Iterable.cpp"

namespace Core
{
    namespace Iterable
    {
        template <typename T>
        class BB : public Iterable<T>
        {

        private:
            // ### Private variables

            size_t _First = 0;

            // ### Private Functions

            _FORCE_INLINE inline T &_ElementAt(size_t Index)
            {
                return this->_Content[(_First + Index) % this->_Capacity];
            }

            _FORCE_INLINE inline const T &_ElementAt(size_t Index) const
            {
                return this->_Content[(_First + Index) % this->_Capacity];
            }

        public:
            // ### Constructors

            BB() : Iterable<T>(), _First(0) {}

            BB(size_t Capacity, bool Growable = true) : Iterable<T>(Capacity, Growable), _First(0) {}

            BB(BB &Other) : Iterable<T>(Other), _First(0) {}

            BB(BB &&Other)
            noexcept : Iterable<T>(std::move(Other)), _First(Other._First) {}

            // ### Public Functions

            void Resize(size_t Size)
            {
                T *_New = new T[Size];

                for (size_t i = 0; i < this->_Length; i++)
                {
                    _New[i] = std::move(_ElementAt(i));
                }

                delete[] this->_Content;

                this->_Content = _New;

                _First = 0;

                this->_Capacity = Size;
            }

            void Remove(size_t Index)
            {
                if (Index >= this->_Length)
                    throw std::out_of_range("");

                this->_Length--;

                if (Index == 0)
                {
                    _ElementAt(0).~T();

                    _First = (_First + 1) % this->_Capacity;
                }
                else if (Index == this->_Length)
                {
                    _ElementAt(Index).~T();
                }
                else
                {
                    for (size_t i = Index; i < this->_Length; i++)
                    {
                        _ElementAt(i) = std::move(_ElementAt(i + 1));
                    }
                }
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

            std::string Peek(size_t Size)
            {
                if (Size > this->_Capacity)
                    throw std::out_of_range("");

                std::string str; // Optimization needed

                str.resize(Size * sizeof(T));

                for (size_t i = 0; i < Size; i++)
                {
                    str += this->_Content[i];
                }

                return str;
            }

            std::string Peek()
            {
                return Peek(this->_Capacity);
            }

            friend std::ostream &operator<<(std::ostream &os, BB &BB)
            {
                while (!BB.IsEmpty())
                {
                    os << BB.Take();
                }

                return os;
            }
        };
    }
}