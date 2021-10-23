#pragma once

#ifndef _FORCE_INLINE
#define _FORCE_INLINE __attribute__((always_inline))
#endif

#include <iostream>
#include <sstream>
#include <functional>
#include <string>
#include <cstring>

namespace Core
{
    namespace Iterable
    {
        template <typename T>
        class Iterable
        {

        protected:
            static_assert(std::is_move_assignable<T>::value, "T must be move assignable");
            static_assert(std::is_copy_assignable<T>::value, "T must be copy assignable");

            // ### Private variables

            size_t _Capacity = 0;
            size_t _Length = 0;
            T *_Content = NULL;
            bool _Growable = true;

            std::function<size_t(size_t, size_t)> _ResizeCallback = [](size_t Current, size_t Minimum) -> size_t
            {
                return (Current * 2) + Minimum;
            };

            // ### Private Functions

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

            virtual _FORCE_INLINE inline T &_ElementAt(size_t Index) { return _Content[Index]; }

            // ### Constructors

            Iterable() : _Capacity(0), _Length(0), _Content(new T[1]), _Growable(true){}

            Iterable(size_t Capacity, bool Growable = true) : _Capacity(Capacity), _Length(0), _Content(new T[Capacity]), _Growable(Growable){}

            Iterable(Iterable &Other) : _Capacity(Other._Capacity), _Length(Other._Length), _Content(new T[Other._Capacity]), _Growable(Other._Growable)
            {
                for (size_t i = 0; i < Other._Length; i++)
                {
                    _Content[i] = Other._Content[(Other._First + i) % Other._Capacity];
                }
            }

            Iterable(Iterable &&Other) noexcept : _Capacity(Other._Capacity), _Length(Other._Length), _Growable(Other._Growable)
            {
                std::swap(_Content, Other._Content);
            }

            // ### Destructor

            ~Iterable() { delete[] _Content; }

        public:
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

            std::function<size_t(size_t, size_t)> OnResize() const
            {
                return _ResizeCallback;
            }

            void OnResize(std::function<size_t(size_t, size_t)> CallBack)
            {
                _ResizeCallback = CallBack;
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

            // ### Public Functions

            // ### Virtual Functions

            virtual void Resize(size_t Size) {}

            virtual void Add(T &&Item)
            {
                _IncreaseCapacity();

                _ElementAt(_Length) = std::move(Item);
                _Length++;
            }

            virtual void Add(const T &Item)
            {
                _IncreaseCapacity();

                _ElementAt(_Length) = Item;
                _Length++;
            }

            virtual void Add(const T &Item, size_t Count)
            {
                _IncreaseCapacity(Count);

                for (size_t i = 0; i < Count; i++) // optimize loop
                {
                    _ElementAt(_Length + i) = Item;
                }

                _Length += Count;
            }

            virtual void Add(T *Items, size_t Count)
            {
                _IncreaseCapacity(Count);

                for (size_t i = 0; i < Count; i++)
                {
                    _ElementAt(_Length + i) = std::move(Items[i]);
                }

                _Length += Count;
            }

            virtual void Add(const T *Items, size_t Count)
            {
                _IncreaseCapacity(Count);

                for (size_t i = 0; i < Count; i++)
                {
                    _ElementAt(_Length + i) = Items[i];
                }

                _Length += Count;
            }

            virtual void Fill(const T &Item)
            {
                for (size_t i = _Length; i < _Capacity; i++)
                {
                    _ElementAt(i) = Item;
                }

                _Length = _Capacity;
            }

            virtual T Take() {}

            virtual void Take(T *Items, size_t Count)
            {
                if (_Length < Count)
                    throw std::out_of_range("");

                for (size_t i = 0; i < Count; i++)
                {
                    Items[i] = Take();
                }
            }

            virtual void Remove(size_t Index) {}

            // ### Pre-defined functions

            void ForEach(std::function<void(const T &)> Action) const
            {
                for (int i = 0; i < _Length; i++)
                {
                    Action(_ElementAt(i));
                }
            }

            void ForEach(std::function<void(int, const T &)> Action) const
            {
                for (int i = 0; i < _Length; i++)
                {
                    Action(i, _ElementAt(i));
                }
            }

            //

            Iterable<T> Where(std::function<bool(const T &)> Condition) const
            {
                Iterable<T> result(_Capacity);

                for (size_t i = 0; i < _Length; i++)
                {
                    T &Item = _ElementAt(i);
                    if (Condition(Item))
                        result.Add(Item);
                }

                return result;
            }

            bool Contains(T Item) const
            {
                Iterable<T> result(_Capacity);

                for (size_t i = 0; i < _Length; i++)
                {
                    if (_ElementAt(i) == Item)
                        return true;
                }

                return false;
            }

            bool Contains(T Item, int &Index) const
            {
                Iterable<T> result(_Capacity);

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

            std::string ToString(size_t Size)
            {
                if (Size > _Length)
                    throw std::out_of_range("");

                std::string str; // Optimization needed

                str.resize(Size * sizeof(T));

                for (size_t i = 0; i < Size; i++)
                {
                    str += _ElementAt(i);
                }

                return str;
            }

            std::string ToString()
            {
                return ToString(_Length);
            }

            // ### Operators

            // ### Pre-defined Operators

            Iterable &operator=(Iterable &Other) = delete;

            Iterable &operator=(Iterable &&Other) noexcept
            {
                if (this == &Other)
                    return *this;

                delete[] _Content;

                _Capacity = Other._Capacity;
                _Length = Other._Length;

                std::swap(_Content, Other._Content);

                return *this;
            }

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

                if (!IsEmpty())
                    Item = Take();

                return *this;
            }

            Iterable &operator<<(T &Item)
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