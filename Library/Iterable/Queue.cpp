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

            Queue() : Iterable<T>(), _First(0) {}

            Queue(size_t Capacity, bool Growable = true) : Iterable<T>(Capacity, Growable), _First(0) {}

            Queue(T *Array, int Count, bool Growable = true) : Iterable<T>(Array, Count, Growable), _First(0) {}

            Queue(const Queue &Other) : Iterable<T>(Other), _First(0) {}

            Queue(Queue &&Other) noexcept : Iterable<T>(std::move(Other)), _First(Other._First) {}

            // ### Destructor

            ~Queue() = default;

            // ### Properties

            Span<T> Chunk()
            {
                return Span<T>(&(this->_Content[_First]), std::min((this->_Capacity - this->_Length), this->_Length), false);
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

                _First = 0;

                this->_Capacity = Size;
            }

            void Add(T &&Item)
            {
                this->_IncreaseCapacity();

                _ElementAt(this->_Length) = std::move(Item);
                this->_Length++;
            }

            void Add(const T &Item)
            {
                this->_IncreaseCapacity();

                _ElementAt(this->_Length) = Item;
                this->_Length++;
            }

            void Add(const T &Item, size_t Count)
            {
                this->_IncreaseCapacity(Count);

                for (size_t i = 0; i < Count; i++) // optimize loop
                {
                    _ElementAt(this->_Length + i) = Item;
                }

                this->_Length += Count;
            }

            void Add(T &&Item, size_t Count)
            {
                this->_IncreaseCapacity(Count);

                for (size_t i = 0; i < Count; i++)
                {
                    _ElementAt(this->_Length + i) = std::forward<T>(Item);
                }

                this->_Length += Count;
            }

            void Add(T *Items, size_t Count)
            {
                this->_IncreaseCapacity(Count);

                for (size_t i = 0; i < Count; i++)
                {
                    _ElementAt(this->_Length + i) = std::move(Items[i]);
                }

                this->_Length += Count;
            }

            void Add(const T *Items, size_t Count)
            {
                this->_IncreaseCapacity(Count);

                for (size_t i = 0; i < Count; i++)
                {
                    _ElementAt(this->_Length + i) = Items[i];
                }

                this->_Length += Count;
            }

            void Remove(size_t Index)
            {
                if (Index >= this->_Length)
                    throw std::out_of_range("");

                this->_Length--;

                if constexpr (std::is_arithmetic<T>::value)
                {
                    for (size_t i = Index; i < this->_Length; i++)
                    {
                        _ElementAt(i) = std::move(_ElementAt(i + 1));
                    }
                }
                else
                {
                    if (Index == this->_Length)
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
            }

            void Fill(const T &Item)
            {
                for (size_t i = this->_Length; i < this->_Capacity; i++)
                {
                    _ElementAt(i) = Item;
                }

                this->_Length = this->_Capacity;
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

                this->_Length -= Count;
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
                this->_Capacity = Other._Capacity;
                _First = Other._First;
                this->_Length = Other._Length;

                std::swap(this->_Content, Other._Content);

                return *this;
            }

            // friend Queue<char> &operator<<(Queue<char> &queue, const std::string &Message)
            // {
            //     queue.Add(Message.c_str(), Message.length());

            //     return queue;
            // }

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