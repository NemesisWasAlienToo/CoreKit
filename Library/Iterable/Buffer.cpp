#pragma once

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
        class Buffer
        {

        private:
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

            size_t _First = 0;

            // ### Private Functions

            void _IncreaseCapacity(size_t Minimum = 1)
            {
                if (_Capacity - _Length >= Minimum)
                    return;

                if (!_Growable)
                    throw std::out_of_range("");

                Resize(_ResizeCallback(_Capacity, Minimum));
            }

        public:
            // ### Constructors

            Buffer() : _Capacity(0), _Length(0), _Content(new T[1]), _Growable(true), _First(0) {}

            Buffer(size_t Capacity, bool Growable = true) : _Capacity(Capacity), _Length(0), _Content(new T[Capacity]), _Growable(Growable), _First(0) {}

            Buffer(Buffer &Other) : _Capacity(Other._Capacity), _Length(Other._Length), _Content(new T[Other._Capacity]), _First(Other._First), _Growable(Other._Growable)
            {
                for (size_t i = 0; i < Other._Length; i++)
                {
                    _Content[i] = Other._Content[(Other._First + i) % Other._Capacity];
                }
            }

            Buffer(Buffer &&Other) noexcept : _Capacity(Other._Capacity), _Length(Other._Length), _First(Other._First), _Growable(Other._Growable)
            {
                std::swap(_Content, Other._Content);
            }

            // ### Destructor

            ~Buffer() { delete[] _Content; }

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

            inline bool IsEmpty() { return _Length == 0; }

            inline bool IsFull() { return _Length == _Capacity; }

            inline size_t IsFree() { return _Capacity - _Length; }

            // ### Public Functions

            void Resize(size_t Size)
            {
                T *_New = new T[Size];

                for (size_t i = 0; i < _Length; i++)
                {
                    _New[i] = std::move(_Content[(_First + i) % _Capacity]);
                }

                delete[] _Content;

                _Content = _New;

                _First = 0;

                _Capacity = Size;
            }

            void Add(T &&Item)
            {
                _IncreaseCapacity();

                _Content[(_First + _Length) % _Capacity] = std::move(Item);
                _Length++;
            }

            void Add(const T &Item)
            {
                _IncreaseCapacity();

                _Content[(_First + _Length) % _Capacity] = Item;
                _Length++;
            }

            void Add(const T &Item, size_t Count)
            {
                _IncreaseCapacity(Count);

                for (size_t i = 0; i < Count; i++) // optimize loop
                {
                    _Content[(_First + _Length + i) % _Capacity] = Item;
                }

                _Length += Count;
            }

            void Add(const T *Items, size_t Count)
            {
                _IncreaseCapacity(Count);

                for (size_t i = 0; i < Count; i++)
                {
                    _Content[(_First + _Length + i) % _Capacity] = Items[i];
                }

                _Length += Count;
            }

            void Add(T *Items, size_t Count)
            {
                _IncreaseCapacity(Count);

                for (size_t i = 0; i < Count; i++)
                {
                    _Content[(_First + _Length + i) % _Capacity] = std::move(Items[i]);
                }

                _Length += Count;
            }

            void Remove(size_t Index)
            {
                if (Index >= _Length)
                    throw std::out_of_range("");

                _Length--;

                if (Index == 0)
                {
                    _Content[_First].~T();

                    _First = (_First + 1) % _Capacity;
                }
                else if (Index == _Length)
                {
                    _Content[(_First + Index) % _Capacity].~T();
                }
                else
                {
                    for (size_t i = Index; i < _Length; i++)
                    {
                        _Content[(_First + i) % _Capacity] = std::move(_Content[(_First + i + 1) % _Capacity]);
                    }
                }
            }

            void Fill(const T &Item)
            {
                size_t Count = _Capacity - _Length;

                for (size_t i = 0; i < Count; i++)
                {
                    Add(Item);
                }
            }

            T Take()
            {
                if (IsEmpty())
                    throw std::out_of_range("");

                T Item = std::move(_Content[_First]); // OK?
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

            void Free()
            {
                while (!IsEmpty())
                {
                    Take();
                }
            }

            void Free(size_t Count)
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

                return _Content[_First];
            }

            T &Last()
            {
                if (IsEmpty())
                    throw std::out_of_range("");

                return _Content[(_First + (_Length - 1)) % _Capacity];
            }

            void ForEach(std::function<void(const T &)> Action) const
            {
                for (int i = 0; i < _Length; i++)
                {
                    Action(_Content[i]);
                }
            }

            void ForEach(std::function<void(int, const T &)> Action) const
            {
                for (int i = 0; i < _Length; i++)
                {
                    Action(i, _Content[(_First + i) % _Capacity]);
                }
            }

            //

            Buffer<T> Where(std::function<bool(const T &)> Condition) const
            {
                Buffer<T> result(_Capacity);

                for (size_t i = 0; i < _Length; i++)
                {
                    T Item = T(_Content[(_First + i) % _Capacity]);
                    if (Condition(Item))
                        result.Add(Item);
                }

                return result;
            }

            bool Contains(T Item) const
            {
                Buffer<T> result(_Capacity);

                for (size_t i = 0; i < _Length; i++)
                {
                    if (_Content[(_First + i) % _Capacity] == Item)
                        return true;
                }

                return false;
            }

            bool Contains(T Item, int &Index) const
            {
                Buffer<T> result(_Capacity);

                for (size_t i = 0; i < _Length; i++)
                {
                    if (_Content[(_First + i) % _Capacity] == Item)
                    {
                        Index = i;
                        return true;
                    }
                }

                return false;
            }

            template <typename O>
            Buffer<O> Map(std::function<O(const T &)> Transform) const
            {
                Buffer<O> result(_Capacity);

                for (size_t i = 0; i < _Length; i++)
                {
                    result.Add(Transform(_Content[(_First + i) % _Capacity]));
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
                    str += _Content[(_First + i) % _Capacity];
                }

                return str;
            }

            std::string ToString()
            {
                return ToString(_Length);
            }

            // ### Operators

            Buffer &operator=(Buffer &Other) = delete;

            Buffer &operator=(Buffer &&Other) noexcept
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

            char &operator[](const size_t &index)
            {
                if (index >= _Length)
                    throw std::out_of_range("");

                return _Content[(_First + index) % _Capacity];
            }

            Buffer &operator>>(T &Item)
            {

                if (!IsEmpty())
                    Item = Take();

                return *this;
            }

            Buffer &operator<<(T &Item)
            {
                Add(Item);

                return *this;
            }

            Buffer &operator<<(T &&Item)
            {
                Add(std::move(Item));

                return *this;
            }

            friend std::ostream &operator<<(std::ostream &os, Buffer &buffer)
            {
                while (!buffer.IsEmpty())
                {
                    os << buffer.Take();
                }

                return os;
            }
        };
    }
}