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
                _Handler return _Content[(_First + Index) % _Capacity];
            }

            _FORCE_INLINE inline const T &_ElementAt(size_t Index) const
            {
                return _Content[(_First + Index) % _Capacity];
            }

        public:
            // ### Constructors

            BB() : Iterable<T>(), _First(0) {}

            BB(size_t Capacity, bool Growable = true) : _Capacity(Capacity), _Length(0), _Content(new T[Capacity]), _Growable(Growable), _First(0) {}

            BB(BB &Other) : _Capacity(Other._Capacity), _Length(Other._Length), _Content(new T[Other._Capacity]), _Growable(Other._Growable), _First(0)
            {
                for (size_t i = 0; i < Other._Length; i++)
                {
                    _ElementAt(i) = Other._ElementAt(i);
                }
            }

            BB(BB &&Other)
            noexcept : _Capacity(Other._Capacity), _Length(Other._Length), _Growable(Other._Growable), _First(Other._First)
            {
                std::swap(_Content, Other._Content);
            }

            // ### Destructor

            ~BB() { delete[] _Content; }

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

            // ### Public Functions

            void Resize(size_t Size)
            {
                T *_New = new T[Size];

                for (size_t i = 0; i < _Length; i++)
                {
                    _New[i] = std::move(_ElementAt(i));
                }

                delete[] _Content;

                _Content = _New;

                _First = 0;

                _Capacity = Size;
            }

            void Add(T &&Item)
            {
                _IncreaseCapacity();

                _ElementAt(_Length) = std::move(Item);
                _Length++;
            }

            void Add(const T &Item)
            {
                _IncreaseCapacity();

                _ElementAt(_Length) = Item;
                _Length++;
            }

            void Add(const T &Item, size_t Count)
            {
                _IncreaseCapacity(Count);

                for (size_t i = 0; i < Count; i++) // optimize loop
                {
                    _ElementAt(_Length + i) = Item;
                }

                _Length += Count;
            }

            void Add(T *Items, size_t Count)
            {
                _IncreaseCapacity(Count);

                for (size_t i = 0; i < Count; i++)
                {
                    _ElementAt(_Length + i) = std::move(Items[i]);
                }

                _Length += Count;
            }

            void Add(const T *Items, size_t Count)
            {
                _IncreaseCapacity(Count);

                for (size_t i = 0; i < Count; i++)
                {
                    _ElementAt(_Length + i) = Items[i];
                }

                _Length += Count;
            }

            void Remove(size_t Index) // Not Compatiable
            {
                if (Index >= _Length)
                    throw std::out_of_range("");

                _Length--;

                if (Index == 0)
                {
                    _ElementAt(0).~T();

                    _First = (_First + 1) % _Capacity;
                }
                else if (Index == _Length)
                {
                    _ElementAt(Index).~T();
                }
                else
                {
                    for (size_t i = Index; i < _Length; i++)
                    {
                        _ElementAt(i) = std::move(_ElementAt(i + 1));
                    }
                }
            }

            void Fill(const T &Item)
            {
                size_t Count = _Capacity - _Length;

                for (size_t i = _Length; i < _Capacity; i++)
                {
                    _ElementAt(i) = Item;
                }

                _Length = _Capacity;
            }

            T Take() // Not Compatiable
            {
                if (IsEmpty())
                    throw std::out_of_range("");

                T Item = std::move(_ElementAt(0)); // OK?
                _Length--;
                _First = (_First + 1) % _Capacity;

                return Item;
            }

            void Take(T *Items, size_t Count)
            {
                if (_Length < Count)
                    throw std::out_of_range("");

                for (size_t i = 0; i < Count; i++)
                {
                    Items[i] = Take();
                }
            }

            void Free() // Not Compatiable
            {
                while (!IsEmpty())
                {
                    Take();
                }
            }

            void Free(size_t Count) // Not Compatiable
            {
                for (size_t i = 0; i < Count; i++)
                {
                    Take();
                }
            }

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

            BB<T> Where(std::function<bool(const T &)> Condition) const
            {
                BB<T> result(_Capacity);

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
                BB<T> result(_Capacity);

                for (size_t i = 0; i < _Length; i++)
                {
                    if (_ElementAt(i) == Item)
                        return true;
                }

                return false;
            }

            bool Contains(T Item, int &Index) const
            {
                BB<T> result(_Capacity);

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
            BB<O> Map(std::function<O(const T &)> Transform) const
            {
                BB<O> result(_Capacity);

                for (size_t i = 0; i < _Length; i++)
                {
                    result.Add(Transform(_ElementAt(i)));
                }

                return result;
            }

            std::string Peek(size_t Size)
            {
                if (Size > _Capacity)
                    throw std::out_of_range("");

                std::string str; // Optimization needed

                str.resize(Size * sizeof(T));

                for (size_t i = 0; i < Size; i++)
                {
                    str += _Content[i];
                }

                return str;
            }

            std::string Peek()
            {
                return Peek(_Capacity);
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

            BB &operator=(BB &Other) = delete;

            BB &operator=(BB &&Other) noexcept
            {
                if (this == &Other)
                    return *this;

                delete[] _Content;

                _Capacity = Other._Capacity;
                _First = Other._First;
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

            BB &operator>>(T &Item)
            {

                if (!IsEmpty())
                    Item = Take();

                return *this;
            }

            BB &operator<<(T &Item)
            {
                Add(Item);

                return *this;
            }

            BB &operator<<(T &&Item)
            {
                Add(std::move(Item));

                return *this;
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