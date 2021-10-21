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
                if (!IsFull())
                    return;

                if (!_Growable)
                    throw std::out_of_range("");

                Resize(_ResizeCallback(_Capacity, Minimum));
            }

        public:
            // ### Constructors

            List() = default;

            List(size_t Capacity, bool Growable = true) : _Content(new T[Capacity]), _Capacity(Capacity), _Length(0), _Growable(Growable) {}

            List(T Array[], int Count, bool Growable = true) : _Content(new T[Count]), _Capacity(Count), _Length(Count), _Growable(Growable)
            {
                for (size_t i = 0; i < Count; i++)
                {
                    _Content[i] = Array[i];
                }
            }

            List(List &Other) : _Content(new T[Other._Capacity]), _Capacity(Other._Capacity), _Length(Other._Length), _Growable(Other._Growable)
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

            std::function<size_t(size_t, size_t)> Growable() const
            {
                return _ResizeCallback;
            }

            void OnResize(std::function<size_t(size_t, size_t)> CallBack){
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

                _First = _Content;

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

            void Add(const T &item)
            {
                _IncreaseCapacity();
                _Content[_Length++] = item;
            }

            void Add(T &&item)
            {
                _IncreaseCapacity();
                _Content[_Length++] = std::move(item);
            }

            void Fill(const T &Item)
            {
                for (size_t i = _Length; i < _Capacity; i++)
                {
                    _Content[i] = Item;
                }
            }

            void Fill(const T &Item, size_t Count)
            {
                for (size_t i = _Length; i < _Capacity; i++)
                {
                    _Content[i] = Item;
                }
            }

            T Take()
            {
                if (_Length == 0)
                    throw std::out_of_range("");

                _Length--;

                return std::move(_Content[_Length]);
            }

            void Remove(size_t Index)
            {
                if (Index > _Length)
                    throw std::out_of_range("");

                _Length--;

                for (size_t i = Index; i < _Length; i++)
                {
                    _Content[i] = std::move(_Content[i + 1]);
                }
            }

            bool Contains(T Item) const
            {
                List<T> result(_Capacity);

                for (T item : _Content)
                {
                    if (item == Item)
                        return true;
                }

                return false;
            }

            bool Contains(T Item, int &Index) const
            {
                List<T> result(_Capacity);

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
                for (int i = 0; i < _Length; i++)
                {
                    Action(_Content[i]);
                }
            }

            List<T> Where(std::function<bool(const T &)> Condition) const
            {
                List<T> result(_Capacity);

                for (T item : _Content)
                {
                    if (Condition(item))
                        result.Add(item);
                }

                return result;
            }

            template <typename O>
            List<O> Map(std::function<O(const T &)> Transform) const
            {
                List<O> result(_Capacity);

                for (T item : _Content)
                {
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

            bool operator==(const List &Other) noexcept
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