#pragma once

#include <iostream>
#include <functional>

namespace Iterable
{
    template <typename T>
    class List
    {
    private:
        // ### Private variables

        T * _Content = NULL;
        int _Capacity = 0;
        int _Length = 0;

        // ### Private functions
        void increaseCapacity()
        {
            if (_Capacity > _Length)
                return;
            if (_Capacity > 0) // ### Change realloc to new due to internal pointer
                _Content = (T *)std::realloc(_Content, sizeof(T) * ++_Capacity);
            if (_Capacity == 0)
                _Content = new T[++_Capacity];
        }

    public:
        // ### Constructors

        List() = default;

        List(int Capacity) : _Content(new T[Capacity]), _Capacity(Capacity), _Length(0) {}

        List(List &Other) : _Content(new T[Other._Capacity]), _Capacity(Other._Capacity), _Length(Other._Length)
        {
            Other.ForEach([&](int Index, T& Item)
                          { _Content[Index] = T(Item); });
        }

        List(List &&Other) noexcept : _Capacity(Other._Capacity), _Length(Other._Length)
        {
            std::swap(_Content, Other._Content);
        }

        // List(T Array[], int Count) : _Content(new T[Count]), _Capacity(Count), _Length(Count)
        // {
            
        // }

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

        void Add(T &item)
        {
            T t(item);
            increaseCapacity();
            _Content[_Length++] = t;
        }

        void Add(T &&item)
        {
            T t(std::move(item));
            increaseCapacity();
            _Content[_Length++] = t;
        }

        void ForEach(std::function<void(int, T&)> Action)
        {
            for (int i = 0; i < _Length; i++)
            {
                Action(i, _Content[i]);
            }
        }

        List<T> Where(std::function<bool(T&)> Condition)
        {
            List<T> result(_Capacity);

            for (T item : _Content)
            {
                if (Condition(item))
                    result.Add(item);
            }

            return result;
        }

        bool Contains(T Item){
            List<T> result(_Capacity);

            for (T item : _Content)
            {
                if (item == Item)
                    return true;
            }

            return false;
        }

        bool Contains(T Item, int& Index){
            List<T> result(_Capacity);

            for (int i = 0 ; i < _Length ; i++)
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

        void From(T Array[], int Count){
            
        }

        void From(List &Other){

        }

        // ### Operators

        T &operator[](int index)
        {
            return _Content[index];
        }

        List &operator=(List &Other) = delete;

        List &operator=(List &&Other) noexcept
        {
            if (this == &Other)
                return *this;

            _Capacity = Other._Capacity;
            _Length = Other._Length;
            std::swap(_Content, Other._Content);

            return *this;
        }

        bool operator==(const List& Other) noexcept{
            return this->_Content == Other->_Content;
        }

        bool operator!=(const List& Other) noexcept{
            return this->_Content != Other->_Content;
        }
    };
}