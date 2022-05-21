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
            static_assert(std::is_move_assignable<T>::value, "T must be at least move assignable");

            // ### Private variables

            size_t _Capacity = 0;
            size_t _Length = 0;
            T *_Content = nullptr;
            bool _Growable = true;

            _FORCE_INLINE inline size_t _CalculateNewSize(size_t Minimum)
            {
                return (_Capacity * 2) + Minimum;
            }

            // ### Pre-defined Functions

            void _IncreaseCapacity(size_t Minimum = 1)
            {
                if (_Capacity - _Length >= Minimum)
                    return;

                if (!_Growable)
                    throw std::out_of_range("");

                Resize(_CalculateNewSize(Minimum - (_Capacity - _Length)));
            }

            // ### Virtual Functions

            _FORCE_INLINE inline virtual T &_ElementAt(size_t Index) { return _Content[Index]; }
            _FORCE_INLINE inline virtual const T &_ElementAt(size_t Index) const { return _Content[Index]; }

        public:
            // ### Constructors

            Iterable() = default;

            Iterable(size_t Capacity, bool Growable = true) : _Capacity(Capacity), _Length(0), _Content(new T[Capacity]), _Growable(Growable) {}

            Iterable(T *Array, size_t Count, bool Growable = true) : _Capacity(Count), _Length(0), _Content(new T[Count]), _Growable(Growable)
            {
                for (size_t i = 0; i < Count; i++)
                {
                    _ElementAt(i) = Array[i];
                }
            }

            Iterable(const Iterable &Other) : _Capacity(Other._Capacity), _Length(Other._Length), _Content(new T[Other._Capacity]), _Growable(Other._Growable)
            {
                for (size_t i = 0; i < Other._Length; i++)
                {
                    _ElementAt(i) = Other._ElementAt(i);
                }
            }

            Iterable(Iterable &&Other) noexcept : _Capacity(Other._Capacity), _Length(Other._Length), _Content(Other._Content), _Growable(Other._Growable)
            {
                Other._Content = nullptr;
                Other._Capacity = 0;
                Other._Length = 0;
                Other._Growable = false;
            }

            template <typename... TArgs>
            Iterable(size_t Size, const TArgs &...Args) : _Length(Size), _Content(new T[Size](std::forward<TArgs>(Args)...)) {}

            template <typename... TArgs>
            Iterable(size_t Size, TArgs &&...Args) : _Length(Size), _Content(new T[Size](std::forward<TArgs>(Args)...)) {}

            // ### Destructor

            ~Iterable()
            {
                delete[] _Content;
                _Content = nullptr;
            }

            // ### Statics

            template <typename TBuilder>
            static Iterable<T> Build(size_t Start, size_t End, TBuilder Builder)
            {
                Iterable<T> result((End - Start) + 1);

                for (size_t i = Start; i <= End; i++)
                {
                    result.Add(Builder(i));
                }

                return result;
            }

            // ### Properties

            inline T *Content() noexcept
            {
                return _Content;
            }

            inline T const *Content() const noexcept
            {
                return _Content;
            }

            inline size_t Length() const noexcept
            {
                return _Length;
            }

            inline void Length(size_t Lenght) noexcept
            {
                _Length = Lenght;
            }

            inline size_t Capacity() const noexcept
            {
                return _Capacity;
            }

            inline bool Growable() const noexcept
            {
                return _Growable;
            }

            inline void Growable(bool Growable) noexcept
            {
                _Growable = Growable;
            }

            inline bool IsEmpty() noexcept { return _Length == 0; }

            inline bool IsFull() noexcept { return _Length == _Capacity; }

            inline size_t IsFree() noexcept { return _Capacity - _Length; }

            T &First()
            {
                return *this[0];
            }

            T &Last()
            {
                return this->operator[](_Length - 1);
            }

            T const &First() const
            {
                return this->operator[](0);
            }

            T const &Last() const
            {
                return *this[_Length - 1];
            }

            // Functions

            void Resize(size_t Size)
            {
                // if(Size != _Capacity)
                // {

                // }

                T *_New = new T[Size];

                for (size_t i = 0; i < this->_Length; i++)
                {
                    _New[i] = std::move(_ElementAt(i));
                }

                delete[] this->_Content;

                this->_Content = _New;

                this->_Capacity = Size;
            }

            // @todo If NoWrap was true, the new size must be continues

            void Reserve(size_t Count, bool NoWrap = false)
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

            void Add(const T &Item, size_t Count)
            {
                this->_IncreaseCapacity(Count);

                for (size_t i = 0; i < Count; i++)
                {
                    _ElementAt(this->_Length + i) = Item;
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

            void Add(const Iterable<T> &Other)
            {
                for (size_t i = 0; i < Other._Length; i++)
                {
                    Add(Other[i]);
                }
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
                    _ElementAt(i) = T(Item);
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

                if (this->_Length != Index)
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

            // ### Pre-defined functions

            // void Sort(bool Increasing = true)
            // {
            //     std::sort(First(), Last());
            // }

            template <class TCallback>
            void ForEach(TCallback Action)
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    Action(_ElementAt(i));
                }
            }

            template <class TCallback>
            Iterable<T> Where(TCallback Condition)
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

            template <class TCallback>
            bool ContainsWhere(TCallback Condition)
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    T &item = _ElementAt(i);

                    if (Condition(item))
                        return true;
                }

                return false;
            }

            template <class TCallback>
            bool ContainsWhere(size_t &First, TCallback Condition)
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    T &item = _ElementAt(i);

                    if (Condition(item))
                    {
                        First = i;
                        return true;
                    }
                }

                return false;
            }

            template <typename O, class TCallback>
            Iterable<O> Map(TCallback Transform)
            {
                Iterable<O> result(_Capacity);

                for (size_t i = 0; i < _Length; i++)
                {
                    result.Add(Transform(_ElementAt(i)));
                }

                return result;
            }

            template <class TCallback>
            void ForEach(TCallback Action) const
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    Action(_ElementAt(i));
                }
            }

            template <class TCallback>
            Iterable<T> Where(TCallback Condition) const
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

            template <class TCallback>
            std::tuple<bool, size_t> ContainsWhere(TCallback Condition) const
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    const T &item = _ElementAt(i);

                    if (Condition(item))
                    {
                        return {true, i};
                    }
                }

                return {false, 0};
            }

            template <typename O, class TCallback>
            Iterable<O> Map(TCallback Transform) const
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

            bool Contains(const T &Item, size_t &Index) const
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

            Iterable &operator=(Iterable &&Other) noexcept
            {
                if (this != &Other)
                {
                    delete[] _Content;

                    _Content = Other._Content;
                    _Capacity = Other._Capacity;
                    _Length = Other._Length;
                    _Growable = Other._Growable;

                    Other._Content = nullptr;
                    Other._Capacity = 0;
                    Other._Length = 0;
                    Other._Growable = false;
                }

                return *this;
            }

            Iterable &operator=(const Iterable &Other) noexcept
            {
                if (this != &Other)
                {
                    delete[] _Content;

                    _Content = new T[Other._Capacity];
                    _Capacity = Other._Capacity;
                    _Length = Other._Length;
                    _Growable = Other._Growable;

                    for (size_t i = 0; i < _Length; i++)
                    {
                        _ElementAt(i) = Other._ElementAt(i);
                    }
                }

                return *this;
            }

            T &operator[](size_t Index)
            {
                if (Index >= _Length)
                    throw std::out_of_range("");

                return _ElementAt(Index);
            }

            const T &operator[](size_t Index) const
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