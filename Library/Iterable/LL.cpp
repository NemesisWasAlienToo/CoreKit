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
        class LL : public Iterable<T>
        {

        private:
            // ### Private Functions

            _FORCE_INLINE inline T &_ElementAt(size_t Index)
            {
                return this->_Content[Index];
            }

            _FORCE_INLINE inline const T &_ElementAt(size_t Index) const
            {
                return this->_Content[Index];
            }

        public:
            // ### Constructors

            LL() : Iterable<T>() {}

            LL(size_t Capacity, bool Growable = true) : Iterable<T>(Capacity, Growable) {}

            LL(LL &Other) : Iterable<T>(Other) {}

            LL(LL &&Other)
            noexcept : Iterable<T>(std::move(Other)) {}

            ~LL()
            {
                delete[] this->_Content;
            }

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

                this->_Capacity = Size;
            }

            void Remove(size_t Index) // Not Compatiable
            {
                if (Index >= this->_Length)
                    throw std::out_of_range("");

                this->_Length--;

                if (Index == this->_Length)
                {
                    _ElementAt(0).~T();
                }
                else
                {
                    for (size_t i = Index; i < this->_Length; i++)
                    {
                        this->_Content[i] = std::move(this->_Content[i + 1]);
                    }
                }
            }

            T Take() // Not Compatiable
            {
                if (this->_Length == 0)
                    throw std::out_of_range("");

                this->_Length--;

                return std::move(_ElementAt(this->_Length));
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

            friend std::ostream &operator<<(std::ostream &os, const LL &list)
            {
                for (size_t i = 0; i < list._Length; i++)
                {
                    os << "[" << i << "] : " << list._ElementAt(i) << '\n';
                }

                return os;
            }

            static LL<T> Build(size_t Start, size_t End, std::function<T(size_t)> Builder)
            {
                LL<T> result((End - Start) + 1);

                for (size_t i = Start; i <= End; i++)
                {
                    result.Add(Builder(i));
                }

                return result;
            }
        };
    }
}