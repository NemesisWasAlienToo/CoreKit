#pragma once

#ifndef _FORCE_INLINE
#define _FORCE_INLINE __attribute__((always_inline))
#endif

#include <iostream>
#include <sstream>
#include <functional>
#include <string>
#include <cstring>
#include <algorithm>

namespace Core
{
    namespace Iterable
    {
        template <typename T>
        class Iterable
        {

        protected:
            static_assert(std::is_move_assignable<T>::value, "T must be move assignable");

            // ### Private variables

            size_t _Capacity = 0;
            size_t _Length = 0;
            T *_Content = NULL;
            bool _Growable = true;

            size_t _ResizeCallback(size_t Current, size_t Minimum)
            {
                return (Current * 2) + Minimum;
            };

            // ### Pre-defined Functions

            void _IncreaseCapacity(size_t Minimum = 1)
            {
                if (_Capacity - _Length >= Minimum)
                    return;

                if (!_Growable)
                    throw std::out_of_range("");

                Resize(_ResizeCallback(_Capacity, Minimum));
            }

            // ### Virtual Functions

            _FORCE_INLINE inline T &_ElementAt(size_t Index) { return _Content[Index]; }
            _FORCE_INLINE inline const T &_ElementAt(size_t Index) const { return _Content[Index]; }

            // ### Constructors

            Iterable() : _Capacity(1), _Length(0), _Content(new T[1]), _Growable(true) {}

            Iterable(size_t Capacity, bool Growable = true) : _Capacity(Capacity), _Length(0), _Content(new T[Capacity]), _Growable(Growable) {}

            Iterable(T *Array, size_t Count, bool Growable = true) : _Capacity(Count), _Length(0), _Content(new T[Count]), _Growable(Growable)
            {
                for (size_t i = 0; i < Count; i++)
                {
                    _ElementAt(i) = Array[i];
                }
            }

            Iterable(const Iterable &Other) : _Capacity(Other._Capacity), _Length(Other._Length), _Content(new T[Other._Capacity]), _Growable(Other._Growable)//, _ResizeCallback(Other._ResizeCallback)
            {
                for (size_t i = 0; i < Other._Length; i++)
                {
                    _ElementAt(i) = Other._ElementAt(i);
                }
            }

            Iterable(Iterable &&Other) noexcept : _Capacity(Other._Capacity), _Length(Other._Length), _Growable(Other._Growable)//, _ResizeCallback(std::move(Other._ResizeCallback))
            {
                std::swap(_Content, Other._Content);
            }

        public:
            // ### Destructor

            ~Iterable()
            {
                delete[] _Content;
                _Content = nullptr;
            }

            // ### Statics

            static Iterable<T> Build(size_t Start, size_t End, std::function<T(size_t)> Builder)
            {
                Iterable<T> result((End - Start) + 1);

                for (size_t i = Start; i <= End; i++)
                {
                    result.Add(Builder(i));
                }

                return result;
            }

            // ### Properties

            T *Content()
            {
                return _Content;
            }

            size_t Length()
            {
                return _Length;
            }

            size_t Capacity()
            {
                return _Capacity;
            }

            bool Growable() const
            {
                return _Growable;
            }

            void Growable(bool CanGrow) noexcept
            {
                _Growable = CanGrow;
            }

            _FORCE_INLINE inline bool IsEmpty() { return _Length == 0; }

            _FORCE_INLINE inline bool IsFull() { return _Length == _Capacity; }

            _FORCE_INLINE inline size_t IsFree() { return _Capacity - _Length; }

            T &First()
            {
                if (IsEmpty())
                    throw std::out_of_range("");

                return _ElementAt(0);
            }

            const T &First() const
            {
                if (IsEmpty())
                    throw std::out_of_range("");

                return _ElementAt(0);
            }

            T &Last()
            {
                if (IsEmpty())
                    throw std::out_of_range("");

                return _ElementAt(_Length - 1);
            }

            const T &Last() const
            {
                if (IsEmpty())
                    throw std::out_of_range("");

                return _ElementAt(_Length - 1);
            }

            // Functions

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

            void Reserver(size_t Count)
            {
                this->_IncreaseCapacity(Count);

                this->_Length += Count;
            }

            void Add(T &&Item)
            {
                this->_IncreaseCapacity();

                _ElementAt(this->_Length) = std::move(Item);
                (this->_Length)++;
            }

            void Add(const T &Item)
            {
                this->_IncreaseCapacity();

                _ElementAt(this->_Length) = Item;
                (this->_Length)++;
            }

            void AddSorted(T &&Item)
            {
                this->_IncreaseCapacity();

                size_t i;

                for (i = 0; i <  this->_Length && _ElementAt(i) < Item ; i++) {}

                Squeeze(std::move(Item), i);
            }

            void AddSorted(const T &Item)
            {
                this->_IncreaseCapacity();

                size_t i;

                for (i = 0; i <  this->_Length && _ElementAt(i) < Item ; i++) {}

                Squeeze(Item, i);
            }

            void Add(const T &Item, size_t Count)
            {
                this->_IncreaseCapacity(Count);

                for (size_t i = 0; i < Count; i++)
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

            void Squeeze(T &&Item, size_t Index)
            {
                if (this->_Length <= Index)
                    throw std::out_of_range("");

                this->_IncreaseCapacity();

                for (size_t i = this->_Length; i > Index; i--)
                {
                    _ElementAt(i) = std::move(_ElementAt(i - 1)); 
                }
                
                _ElementAt(Index) = std::move(Item);

                (this->_Length)++;
            }

            void Squeeze(const T &Item, size_t Index)
            {
                if (this->_Length <= Index)
                    throw std::out_of_range("");

                this->_IncreaseCapacity();

                for (size_t i = this->_Length; i > Index; i--)
                {
                    _ElementAt(i) = std::move(_ElementAt(i - 1)); 
                }
                
                _ElementAt(Index) = Item;

                (this->_Length)++;
            }

            void Remove(size_t Index)
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
                if (this->_Length == 0)
                    throw std::out_of_range("");

                this->_Length--;

                return std::move(_ElementAt(this->_Length));
            }

            void Take(T *Items, size_t Count)
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

            T Swap(size_t Index)
            {
                if (Index >= this->_Length)
                    throw std::out_of_range("");

                --(this->_Length);

                auto item = std::move(_ElementAt(Index));

                [[likely]] if (this->_Length != Index)
                {
                    _ElementAt(Index) = std::move(_ElementAt(this->_Length));
                }

                return item;
            }

            void Swap(size_t First, size_t Second)
            {
                if (First >= this->_Length || Second >= this->_Length)
                    throw std::out_of_range("");

                std::swap(_ElementAt(First), _ElementAt(Second));
            }

            void Trim(size_t Additional = 0)
            {
                if (this->_Length <= 0)
                    return;

                Resize(Length() + Additional);
            }

            // // ### To be implemented

            // void Add(const Iterable<T> &Other) {}

            // ### Pre-defined functions

            // void Sort(bool Increasing = true)
            // {
            //     std::sort(First(), Last());
            // }

            void ForEach(std::function<void(T &)> Action)
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    Action(_ElementAt(i));
                }
            }

            void ForEach(std::function<void(size_t, T &)> Action)
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    Action(i, _ElementAt(i));
                }
            }

            Iterable<T> Where(std::function<bool(T &)> Condition)
            {
                Iterable<T> result(_Capacity);

                for (size_t i = 0; i < _Length; i++)
                {
                    T &item = _ElementAt(i);

                    if (Condition(item))
                        result.Add(item);
                }

                return result;
            }

            size_t CountWhere(std::function<bool(T &)> Condition)
            {
                size_t result;

                for (size_t i = 0; i < _Length; i++)
                {
                    T &item = _ElementAt(i);

                    if (Condition(item))
                        result++;
                }

                return result;
            }

            bool ContainsWhere(std::function<bool(T &)> Condition)
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    T &item = _ElementAt(i);

                    if (Condition(item))
                        return true;
                }

                return false;
            }

            bool ContainsWhere(size_t &First, std::function<bool(T &)> Condition)
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    T &item = _ElementAt(i);

                    if (Condition(item))
                        return true;
                }

                return false;
            }

            T &FirstWhere(std::function<bool(T &)> Condition)
            {
                size_t result;

                for (size_t i = 0; i < _Length; i++)
                {
                    T &item = _ElementAt(i);

                    if (Condition(item))
                        return _ElementAt(i);
                }

                throw std::out_of_range("No such item");
            }

            T &Biggest() const
            {
                if (this->_Length <= 0)
                    throw std::out_of_range("Instance contains no element");

                size_t result = 0;

                for (size_t i = 1; i < _Length; i++)
                {
                    if (_ElementAt(i) > _ElementAt(result))
                        result = i;
                }

                return _ElementAt(result);
            }

            T &Smallest() const
            {
                if (this->_Length <= 0)
                    throw std::out_of_range("Instance contains no element");

                size_t result = 0;

                for (size_t i = 1; i < _Length; i++)
                {
                    if (_ElementAt(i) < _ElementAt(result))
                        result = i;
                }

                return _ElementAt(result);
            }

            // T &LastWhere(std::function<bool(T &, T &)> Comparer)
            // {
            //     if (this->_Length <= 0)
            //         throw std::out_of_range("Instance contains no element");

            //     size_t result = 0;

            //     for (size_t i = 1; i < _Length; i++)
            //     {
            //         if (Comparer(_ElementAt(result), _ElementAt(i)))
            //             result = i;
            //     }

            //     return _ElementAt(result);
            // }

            // T &LastWhere(std::function<bool(const T &, const T &)> Comparer) const
            // {
            //     if (this->_Length <= 0)
            //         throw std::out_of_range("Instance contains no element");

            //     size_t result = 0;

            //     for (size_t i = 1; i < _Length; i++)
            //     {
            //         if (Comparer(_ElementAt(result), _ElementAt(i)))
            //             result = i;
            //     }

            //     return _ElementAt(result);
            // }

            template <typename O>
            Iterable<O> Map(std::function<O(T &)> Transform)
            {
                Iterable<O> result(_Capacity);

                for (size_t i = 0; i < _Length; i++)
                {
                    result.Add(Transform(_ElementAt(i)));
                }

                return result;
            }

            void ForEach(std::function<void(const T &)> Action) const
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    Action(_ElementAt(i));
                }
            }

            void ForEach(std::function<void(size_t, const T &)> Action) const
            {
                for (int i = 0; i < _Length; i++)
                {
                    Action(i, _ElementAt(i));
                }
            }

            Iterable<T> Where(std::function<bool(const T &)> Condition) const
            {
                Iterable<T> result(_Capacity);

                for (size_t i = 0; i < _Length; i++)
                {
                    const T &item = _ElementAt(i);

                    if (Condition(item))
                        result.Add(item);
                }

                return result;
            }

            size_t CountWhere(std::function<bool(const T &)> Condition) const
            {
                size_t result;

                for (size_t i = 0; i < _Length; i++)
                {
                    const T &item = _ElementAt(i);

                    if (Condition(item))
                        result++;
                }

                return result;
            }

            bool ContainsWhere(std::function<bool(const T &)> Condition) const
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    const T &item = _ElementAt(i);

                    if (Condition(item))
                    {
                        return true;
                    }
                }

                return false;
            }

            bool ContainsWhere(std::function<bool(const T &)> Condition, size_t &First) const
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    const T &item = _ElementAt(i);

                    if (Condition(item))
                    {
                        First = i;
                        return true;
                    }
                }

                return false;
            }

            template <typename O>
            Iterable<O> Map(std::function<O(const T &)> Transform) const
            {
                Iterable<O> result(_Capacity);

                for (size_t i = 0; i < _Length; i++)
                {
                    result.Add(Transform(_ElementAt(i)));
                }

                return result;
            }

            bool Contains(const T &Item) const
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    if (_ElementAt(i) == Item)
                        return true;
                }

                return false;
            }

            bool Contains(const T &Item, int &Index) const
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    if (_ElementAt(i) == Item)
                    {
                        Index = i;
                        return true;
                    }
                }

                return false;
            }

            // ### Operators

            // ### Pre-defined Operators

            T &operator[](const size_t &Index)
            {
                if (Index >= _Length)
                    throw std::out_of_range("");

                return _ElementAt(Index);
            }

            const T &operator[](const size_t &Index) const
            {
                if (Index >= _Length)
                    throw std::out_of_range("");

                return _ElementAt(Index);
            }

            Iterable &operator>>(T &Item)
            {

                Item = Take();

                return *this;
            }

            Iterable &operator<<(const T &Item)
            {
                Add(Item);

                return *this;
            }

            Iterable &operator<<(T &&Item)
            {
                Add(std::move(Item));

                return *this;
            }
        };
    }
}