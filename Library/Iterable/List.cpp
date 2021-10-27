#pragma once

#ifndef _FORCE_INLINE
#define _FORCE_INLINE __attribute__((always_inline))
#endif

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

            void _IncreaseCapacity(size_t Minimum = 1)
            {
                if (_Capacity - _Length >= Minimum)
                    return;

                if (!_Growable)
                    throw std::out_of_range("");

                Resize(_ResizeCallback(_Capacity, Minimum));
            }

            _FORCE_INLINE inline T &_ElementAt(size_t Index)
            {
                return _Content[Index];
            }

            _FORCE_INLINE inline const T &_ElementAt(size_t Index) const
            {
                return _Content[Index];
            }

        public:
            // ### Constructors

            List() : _Capacity(1), _Length(0), _Content(new T[1]), _Growable(true) {}

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

            int Length() noexcept
            {
                return _Length;
            }

            size_t Capacity() noexcept
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

            bool Growable() const noexcept
            {
                return _Growable;
            }

            void Growable(bool CanGrow) noexcept
            {
                _Growable = CanGrow;
            }

            _FORCE_INLINE inline bool IsEmpty() noexcept { return _Length == 0; }

            _FORCE_INLINE inline bool IsFull() noexcept { return _Length == _Capacity; }

            _FORCE_INLINE inline size_t IsFree() noexcept { return _Capacity - _Length; }

            // ### Public Functions

            void Resize(size_t Size)
            {
                if constexpr (std::is_arithmetic<T>::value)
                {
                    _Content = (T *)std::realloc(_Content, Size);
                }
                else
                {
                    T *_New = new T[Size];

                    for (size_t i = 0; i < _Length; i++)
                    {
                        _New[i] = std::move(_ElementAt(i));
                    }

                    delete[] _Content;

                    _Content = _New;
                }

                _Capacity = Size;
            }

            T &First()
            {
                if (_Length <= 0)
                    throw std::out_of_range("");

                return _ElementAt(0);
            }

            const T &First() const
            {
                if (_Length <= 0)
                    throw std::out_of_range("");

                return _ElementAt(0);
            }

            T &Last()
            {
                if (_Length <= 0)
                    throw std::out_of_range("");

                return _ElementAt(_Length - 1);
            }

            const T &Last() const
            {
                if (_Length <= 0)
                    throw std::out_of_range("");

                return _ElementAt(_Length - 1);
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

                for (size_t i = 0; i < Count; i++)
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

                if constexpr (std::is_arithmetic<T>::value)
                {
                    if (Index != _Length)
                    {
                        for (size_t i = Index; i < _Length; i++)
                        {
                            _ElementAt(i) = _ElementAt(i + 1);
                        }
                    }
                }
                else
                {
                    if (Index == _Length)
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
            }

            void Fill(const T &Item)
            {
                for (size_t i = _Length; i < _Capacity; i++)
                {
                    _ElementAt(i) = Item;
                }

                _Length = _Capacity;
            }

            T Take() // Not Compatiable
            {
                if (_Length == 0)
                    throw std::out_of_range("");

                _Length--;

                return std::move(_ElementAt(_Length));
            }

            void Take(T *Items, size_t Count)
            {
                if (_Length < Count)
                    throw std::out_of_range("");

                size_t _Length_ = _Length - Count;

                for (size_t i = _Length_; i < _Length; i++)
                {
                    Items[i] = std::move(_ElementAt(i));
                }

                _Length = _Length_;
            }

            void Swap(size_t Index) // Not Compatiable
            {
                if (Index >= _Length)
                    throw std::out_of_range("");

                if constexpr (std::is_arithmetic<T>::value)
                {
                    if (--_Length != Index)
                    {
                        _ElementAt(Index) = std::move(_ElementAt(_Length));
                    }
                }
                else
                {
                    if (--_Length == Index)
                    {
                        _ElementAt(_Length).~T();
                    }
                    else
                    {
                        _ElementAt(Index) = std::move(_ElementAt(_Length));
                    }
                }
            }

            void Swap(size_t First, size_t Second) // Not Compatiable
            {
                if (First >= _Length || Second >= _Length)
                    throw std::out_of_range("");

                std::swap(_ElementAt(First), _ElementAt(Second));
            }

            bool Contains(const T& Item) const
            {
                for (int i = 0; i < _Length; i++)
                {
                    if (_ElementAt(i) == Item)
                        return true;
                }

                return false;
            }

            bool Contains(const T& Item, int &Index) const
            {
                for (int i = 0; i < _Length; i++)
                {
                    if (_ElementAt(i) == Item)
                    {
                        Index = i;
                        return true;
                    }
                }

                return false;
            }

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

            List<T> Where(std::function<bool(T &)> Condition)
            {
                List<T> result(_Capacity);

                for (size_t i = 0; i < _Length; i++)
                {
                    const T &item = _ElementAt(i);

                    if (Condition(item))
                        result.Add(item);
                }

                return result;
            }

            template <typename O>
            List<O> Map(std::function<O(T &)> Transform)
            {
                List<O> result(_Capacity);

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

            List<T> Where(std::function<bool(const T &)> Condition) const
            {
                List<T> result(_Capacity);

                for (size_t i = 0; i < _Length; i++)
                {
                    const T &item = _ElementAt(i);

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
                    result.Add(Transform(_ElementAt(i)));
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

            // ### Static Functions

            static List<T> Build(size_t Start, size_t End, std::function<T(size_t)> Builder)
            {
                List<T> result((End - Start) + 1);

                for (size_t i = Start; i <= End; i++)
                {
                    result.Add(Builder(i));
                }

                return result;
            }

            // ### Operators

            T &operator[](size_t Index)
            {
                if (Index >= _Length)
                    throw std::out_of_range("");

                return _Content[Index];
            }

            const T &operator[](size_t Index) const
            {
                if (Index >= _Length)
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

            List &operator>>(T &Item)
            {

                Item = Take();

                return *this;
            }

            List &operator<<(const T &Item)
            {
                Add(Item);

                return *this;
            }

            List &operator<<(T &&Item)
            {
                Add(std::move(Item));

                return *this;
            }

            friend std::ostream &operator<<(std::ostream &os, const List &list)
            {
                for (size_t i = 0; i < list._Length; i++)
                {
                    os << "[" << i << "] : " << list._ElementAt(i) << '\n';
                }

                return os;
            }
        };
    }
}