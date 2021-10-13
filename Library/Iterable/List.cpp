#pragma once

#include <iostream>
#include <functional>

namespace Core
{
    namespace Iterable
    {
        template <typename T>
        class List
        {
        private:
            // ### Private variables

            T *_Content = NULL;
            size_t _Capacity = 0;
            size_t _Length = 0;

            // ### Private functions
            void increaseCapacity()
            {
                if (_Capacity > _Length)
                    return;

                if (_Capacity > 0)
                {
                    _Capacity *= 2;

                    T *_Content_ = new T[_Capacity]; // ## Needs smart growth size optimization

                    for (size_t i = 0; i < _Length; i++)
                    {
                        _Content_[i] = T(_Content[i]);
                    }

                    delete[] _Content;
                    _Content = _Content_;
                }

                if (_Capacity == 0)
                    _Content = new T[++_Capacity];
            }

        public:
            // ### Constructors

            List() = default;

            List(size_t Capacity) : _Content(new T[Capacity]), _Capacity(Capacity), _Length(0) {}

            List(T Array[], int Count) : _Content(new T[Count]), _Capacity(Count), _Length(Count)
            {
                for (size_t i = 0; i < Count; i++)
                {
                    _Content[i] = T(Array[i]); // Invoke copy constructor of T
                }
            }

            List(List &Other) : _Content(new T[Other._Capacity]), _Capacity(Other._Capacity), _Length(Other._Length)
            {
                Other.ForEach([&](int Index, T &Item)
                              { _Content[Index] = T(Item); });
            }

            List(List &&Other) noexcept : _Capacity(Other._Capacity), _Length(Other._Length)
            {
                std::swap(_Content, Other._Content);
            }

            // ### Destructor

            ~List()
            {
                delete[] _Content;
                _Content = NULL;
            }

            // ### Properties

            int Length()
            {
                return _Length;
            }

            // ### Utilities

            T &First()
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

            void Add(T &item)
            {
                increaseCapacity();
                _Content[_Length++] = T(item);
            }

            void Add(T &&item)
            {
                T t(std::move(item));
                increaseCapacity();
                _Content[_Length++] = t;
            }

            void ForEach(std::function<void(int, T &)> Action)
            {
                for (int i = 0; i < _Length; i++)
                {
                    Action(i, _Content[i]);
                }
            }

            void ForEach(std::function<void(T &)> Action)
            {
                for (int i = 0; i < _Length; i++)
                {
                    Action(_Content[i]);
                }
            }

            List<T> Where(std::function<bool(T &)> Condition)
            {
                List<T> result(_Capacity);

                for (T item : _Content)
                {
                    if (Condition(item))
                        result.Add(item);
                }

                return result;
            }

            bool Contains(T Item)
            {
                List<T> result(_Capacity);

                for (T item : _Content)
                {
                    if (item == Item)
                        return true;
                }

                return false;
            }

            bool Contains(T Item, int &Index)
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

            // Remove

            // Remove Where

            template <typename O>
            List<O> map(std::function<O(T)> Transform)
            {
                List<O> result(_Capacity);

                for (T item : _Content)
                {
                    result.Add(Transform(item));
                }

                return result;
            }

            void From(T Array[], int Count)
            {
            }

            void From(List &Other)
            {
            }

            // ### Operators

            T &operator[](size_t Index)
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
        };
    }
}