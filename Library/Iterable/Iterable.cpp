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
            virtual _FORCE_INLINE inline const T &_ElementAt(size_t Index) const { return _Content[Index]; }

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

            Iterable(Iterable &Other) : _Capacity(Other._Capacity), _Length(Other._Length), _Content(new T[Other._Capacity]), _Growable(Other._Growable)
            {
                for (size_t i = 0; i < Other._Length; i++)
                {
                    _ElementAt(i) = Other._ElementAt(i);
                }
            }

            Iterable(Iterable &&Other) noexcept : _Capacity(Other._Capacity), _Length(Other._Length), _Growable(Other._Growable)
            {
                std::swap(_Content, Other._Content);
            }

            // ### Destructor

            ~Iterable()
            {
                delete[] _Content;
                _Content = nullptr;
            }

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

            virtual void Resize(size_t Size) {}

            virtual void Add(T &&Item) {}

            virtual void Add(const T &Item) {}

            virtual void Add(const T &Item, size_t Count) {}

            virtual void Add(T *Items, size_t Count) {}

            virtual void Add(const T *Items, size_t Count) {}

            virtual void Fill(const T &Item) {}

            virtual T Take() {return T();}

            virtual void Take(T *Items, size_t Count) {}

            virtual void Remove(size_t Index) {}

            virtual void Swap(size_t Index) {}

            virtual void Swap(size_t First, size_t Second){}

            // Squeeze somthing in between
            // virtual void Squeeze(T Item, size_t Index)

            // ### Pre-defined functions

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
                    const T &item = _ElementAt(i);

                    if (Condition(item))
                        result.Add(item);
                }

                return result;
            }

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