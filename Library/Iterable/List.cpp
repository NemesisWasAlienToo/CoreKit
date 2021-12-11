#pragma once

#include <iostream>
#include <sstream>
#include <functional>

#include "Iterable.cpp"

namespace Core
{
    namespace Iterable
    {
        template <typename T>
        class List : public Iterable<T>
        {
        protected:
            // ### Private Functions

            _FORCE_INLINE inline T &_ElementAt(size_t Index) override
            {
                return this->_Content[Index];
            }

            _FORCE_INLINE inline const T &_ElementAt(size_t Index) const override
            {
                return this->_Content[Index];
            }

        public:
            // ### Constructors

            List() : Iterable<T>() {}

            List(size_t Capacity, bool Growable = true) : Iterable<T>(Capacity, Growable) {}

            List(T *Array, int Count, bool Growable = true) : Iterable<T>(Array, Count, Growable) {}

            List(List &Other) : Iterable<T>(Other) {}

            List(List &&Other) noexcept : Iterable<T>(std::move(Other)) {}

            // ### Properties

            // ### Public Functions

            void Resize(size_t Size) override
            {
                if constexpr (std::is_arithmetic<T>::value)
                {
                    this->_Content = (T *)std::realloc(this->_Content, Size);
                }
                else
                {
                    T *_New = new T[Size];

                    for (size_t i = 0; i < this->_Length; i++)
                    {
                        _New[i] = std::move(_ElementAt(i));
                    }

                    delete[] this->_Content;

                    this->_Content = _New;
                }

                this->_Capacity = Size;
            }

            void Add(T &&Item) override
            {
                this->_IncreaseCapacity();

                _ElementAt(this->_Length) = std::move(Item);
                (this->_Length)++;
            }

            void Add(const T &Item) override
            {
                this->_IncreaseCapacity();

                _ElementAt(this->_Length) = Item;
                (this->_Length)++;
            }

            void Add(const T &Item, size_t Count) override
            {
                this->_IncreaseCapacity(Count);

                for (size_t i = 0; i < Count; i++)
                {
                    _ElementAt(this->_Length + i) = Item;
                }

                this->_Length += Count;
            }

            void Add(T *Items, size_t Count) override
            {
                this->_IncreaseCapacity(Count);

                for (size_t i = 0; i < Count; i++)
                {
                    _ElementAt(this->_Length + i) = std::move(Items[i]);
                }

                this->_Length += Count;
            }

            void Add(const T *Items, size_t Count) override
            {
                this->_IncreaseCapacity(Count);

                for (size_t i = 0; i < Count; i++)
                {
                    _ElementAt(this->_Length + i) = Items[i];
                }

                this->_Length += Count;
            }

            void Remove(size_t Index) override
            {
                if (Index >= this->_Length)
                    throw std::out_of_range("");

                this->_Length--;

                if constexpr (std::is_arithmetic<T>::value)
                {
                    if (Index != this->_Length)
                    {
                        for (size_t i = Index; i < this->_Length; i++)
                        {
                            _ElementAt(i) = _ElementAt(i + 1);
                        }
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

            void Fill(const T &Item) override
            {
                for (size_t i = this->_Length; i < this->_Capacity; i++)
                {
                    _ElementAt(i) = Item;
                }

                this->_Length = this->_Capacity;
            }

            T Take() override
            {
                if (this->_Length == 0)
                    throw std::out_of_range("");

                this->_Length--;

                return std::move(_ElementAt(this->_Length));
            }

            void Take(T *Items, size_t Count) override
            {
                if (this->_Length < Count)
                    throw std::out_of_range("");

                size_t _Length_ = this->_Length - Count;

                for (size_t i = _Length_; i < this->_Length; i++)
                {
                    Items[i] = std::move(_ElementAt(i));
                }

                this->_Length = _Length_;
            }

            void Swap(size_t Index) override
            {
                if (Index >= this->_Length)
                    throw std::out_of_range("");

                if constexpr (std::is_arithmetic<T>::value)
                {
                    if (--(this->_Length) != Index)
                    {
                        _ElementAt(Index) = std::move(_ElementAt(this->_Length));
                    }
                }
                else
                {
                    if (--(this->_Length) == Index)
                    {
                        _ElementAt(this->_Length).~T();
                    }
                    else
                    {
                        _ElementAt(Index) = std::move(_ElementAt(this->_Length));
                    }
                }
            }

            void Swap(size_t First, size_t Second) override
            {
                if (First >= this->_Length || Second >= this->_Length)
                    throw std::out_of_range("");

                std::swap(_ElementAt(First), _ElementAt(Second));
            }

            std::string ToString()
            {
                std::stringstream ss;

                for (size_t i = 0; i < this->_Length; i++)
                {
                    ss << this->_Content[i] << '\n';
                }

                return ss.str();
            }

            // ### Static Functions

            static List<T> Build(size_t Start, size_t End, std::function<T(size_t)> Builder)
            {
                List<T> result((End - Start) + 1);

                for (size_t i = Start; i <= End; i++)
                {
                    result.Add(Builder(i));
                }

                return result;
            }

            // ### Operators

            List &operator=(const List &Other)
            {
                this->_Capacity = Other._Capacity;
                this->_Length = Other._Length;
                this->_Growable = Other._Growable;

                delete[] this->_Content;

                this->_Content = new T[Other._Capacity];

                for (size_t i = 0; i < Other._Length; i++)
                {
                    this->_Content[i] = Other._ElementAt(i);
                }
            }

            List &operator=(List &&Other) noexcept
            {
                if (this == &Other)
                    return *this;

                this->_Capacity = Other._Capacity;
                this->_Length = Other._Length;
                std::swap(this->_Content, Other._Content);

                return *this;
            }

            bool operator==(const List &Other) noexcept // Add more
            {
                return this->_Content == Other->_Content;
            }

            bool operator!=(const List &Other) noexcept
            {
                return this->_Content != Other->_Content;
            }

            friend std::ostream &operator<<(std::ostream &os, const List &List)
            {
                // Check for << opeartor

                for (size_t i = 0; i < List._Length; i++)
                {
                    os << "[" << i << "] : " << List._ElementAt(i) << '\n';
                }

                return os;
            }
        };
    }
}