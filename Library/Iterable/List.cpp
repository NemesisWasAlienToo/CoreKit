#pragma once

#include <iostream>
#include <sstream>
#include <functional>

namespace Core
{
    namespace Iterable
    {
        template <typename T>
        class List
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

            List() : _Capacity(0), _Length(0), _Content(new T[1]), _Growable(true) {}

            List(size_t Capacity, bool Growable = true) : _Capacity(Capacity), _Length(0), _Content(new T[Capacity]), _Growable(Growable) {}

            List(T *Array, int Count, bool Growable = true) : _Capacity(Count), _Length(Count), _Content(new T[Count]), _Growable(Growable)
            {
                for (size_t i = 0; i < Count; i++)
                {
                    _Content[i] = Array[i];
                }
            }

            List(List &Other) : _Capacity(Other._Capacity), _Length(Other._Length), _Content(new T[Other._Capacity]), _Growable(Other._Growable)
            {
                for (size_t i = 0; i < Other._Length; i++)
                {
                    _Content[i] = Other._Content[i];
                }
            }

            List(List &&Other) noexcept : _Capacity(Other._Capacity), _Length(Other._Length), _Growable(Other._Growable)
            {
                std::swap(_Content, Other._Content);
            }

            // ### Destructor

            ~List()
            {
                delete[] _Content;
            }

            // ### Properties

            int Length()
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

            // ### Public Functions

            void Resize(size_t Size)
            {
                T *_New = new T[Size];

                for (size_t i = 0; i < _Length; i++)
                {
                    _New[i] = std::move(_Content[i]);
                }

                delete[] _Content;

                _Content = _New;

                _Capacity = Size;
            }

            T &First()
            {
                if (_Length <= 0)
                    throw std::out_of_range("");

                return _Content[0];
            }

            const T &First() const
            {
                if (_Length <= 0)
                    throw std::out_of_range("");

                return _Content[0];
            }

            T &Last()
            {
                if (_Length <= 0)
                    throw std::out_of_range("");

                return _Content[_Length - 1];
            }

            const T &Last() const
            {
                if (_Length <= 0)
                    throw std::out_of_range("");

                return _Content[_Length - 1];
            }

            void Add(T &&item)
            {
                _IncreaseCapacity();
                _Content[_Length++] = std::move(item);
            }

            void Add(const T &item)
            {
                _IncreaseCapacity();
                _Content[_Length++] = item;
            }

            void Add(const T &Item, size_t Count)
            {
                _IncreaseCapacity(Count);

                for (size_t i = 0; i < Count; i++)
                {
                    _Content[_Length + i] = Item;
                }

                _Length += Count;
            }

            void Add(T *Items, size_t Count)
            {
                _IncreaseCapacity(Count);

                for (size_t i = 0; i < Count; i++)
                {
                    _Content[_Length + i] = std::move(Items[i]);
                }

                _Length += Count;
            }

            void Add(const T *Items, size_t Count)
            {
                _IncreaseCapacity(Count);

                for (size_t i = 0; i < Count; i++)
                {
                    _Content[_Length + i] = Items[i];
                }

                _Length += Count;
            }

            void Fill(const T &Item)
            {
                for (size_t i = _Length; i < _Capacity; i++)
                {
                    _Content[i] = Item;
                }

                _Length = _Capacity;
            }

            void Remove(size_t Index)
            {
                if (Index >= _Length)
                    throw std::out_of_range("");

                _Length--;

                if (Index == _Length)
                {
                    _Content[Index].~T();
                }
                else
                {
                    for (size_t i = Index; i < _Length; i++)
                    {
                        _Content[i] = std::move(_Content[i + 1]);
                    }
                }
            }

            T Take()
            {
                if (_Length == 0)
                    throw std::out_of_range("");

                _Length--;

                return std::move(_Content[_Length]);
            }

            void Take(T *Items, size_t Count) // Optimize this
            {
                if (_Length < Count)
                    throw std::out_of_range("");

                for (size_t i = 0; i < Count; i++)
                {
                    Items[i] = Take();
                }
            }

            bool Contains(T Item) const
            {
                for (int i = 0; i < _Length; i++)
                {
                    if (_Content[i] == Item)
                        return true;
                }

                return false;
            }

            bool Contains(T Item, int &Index) const
            {
                for (int i = 0; i < _Length; i++)
                {
                    if (_Content[i] == Item)
                    {
                        Index = i;
                        return true;
                    }
                }

                return false;
            }

            void ForEach(std::function<void(int, const T &)> Action) const
            {
                for (int i = 0; i < _Length; i++)
                {
                    Action(i, _Content[i]);
                }
            }

            void ForEach(std::function<void(const T &)> Action) const
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    Action(_Content[i]);
                }
            }

            List<T> Where(std::function<bool(const T &)> Condition) const
            {
                List<T> result(_Capacity);

                for (size_t i = 0; i < _Length; i++)
                {
                    T &item = _Content[i];

                    if (Condition(item))
                        result.Add(item);
                }

                return result;
            }

            template <typename O>
            List<O> Map(std::function<O(const T &)> Transform) const
            {
                List<O> result(_Capacity);

                for (size_t i = 0; i < _Length; i++)
                {
                    T &item = _Content[i];

                    result.Add(Transform(item));
                }

                return result;
            }

            std::string ToString()
            {
                std::stringstream ss;

                for (size_t i = 0; i < _Length; i++)
                {
                    ss << _Content[i] << '\n';
                }

                return ss.str();
            }

            // ### Operators

            T &operator[](size_t Index)
            {
                if (Index > _Length)
                    throw std::out_of_range("");

                return _Content[Index];
            }

            const T &operator[](size_t Index) const
            {
                if (Index > _Length)
                    throw std::out_of_range("");

                return _Content[Index];
            }

            List &operator=(List &Other) = delete;

            List &operator=(List &&Other) noexcept
            {
                if (this == &Other)
                    return *this;

                delete[] _Content;

                _Capacity = Other._Capacity;
                _Length = Other._Length;
                std::swap(_Content, Other._Content);

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

            friend std::ostream &operator<<(std::ostream &os, const List &list)
            {
                list.ForEach([os](const T &Item)
                             { os << Item << std::endl; });

                return os;
            }
        };
    }
}