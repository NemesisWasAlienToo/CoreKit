#pragma once

#include <tuple>
#include <memory>
#include <optional>
#include <type_traits>
#include <initializer_list>
#include <sys/uio.h>

#include <Iterable/MemoryHolder.hpp>

namespace Core::Iterable
{
    template <typename T, typename TAllocator = std::allocator<T>>
    class List final
    {
    public:
        // Constructors

        constexpr List() = default;
        constexpr List(size_t Size, bool Growable = true) : _Content(Size), _Length(0), _Growable(Growable) {}
        constexpr List(std::initializer_list<T> list) : _Content(list.size()), _Length(0), _Growable(true)
        {
            for (auto &Item : list)
                Add(Item);
        }

        constexpr List(List const &Other) : _Content(Other._Content), _Length(Other._Length), _Growable(Other._Growable)
        {
            for (size_t i = 0; i < _Length; i++)
            {
                std::construct_at(&_Content[i], Other._Content[i]);
            }
        }

        constexpr List(List &&Other) : _Content(std::move(Other._Content)), _Length(Other._Length), _Growable(Other._Growable)
        {
            Other._Length = 0;
            Other._Growable = true;
        }

        constexpr ~List()
        {
            Free();
        }

        // Operators

        constexpr List &operator=(List const &Other)
        {
            if (this != &Other)
            {
                _Content = Other._Content;
                _Length = Other._Length;
                _Growable = Other._Growable;

                for (size_t i = 0; i < _Length; i++)
                {
                    _Content[i] = Other._Content[i];
                }
            }

            return *this;
        }

        constexpr List &operator=(List &&Other)
        {
            if (this != &Other)
            {
                _Content = std::move(Other._Content);
                _Length = Other._Length;
                _Growable = Other._Growable;

                Other._Length = 0;
                Other._Growable = true;
            }

            return *this;
        }

        constexpr T &operator[](size_t Index)
        {
            if (Index >= _Length)
                throw std::out_of_range("Index out of range");

            return _Content[Index];
        }

        constexpr T const &operator[](size_t Index) const
        {
            if (Index >= _Length)
                throw std::out_of_range("Index out of range");

            return _Content[Index];
        }

        // Peroperties

        constexpr inline size_t Capacity() const
        {
            return _Content.Length();
        }

        constexpr inline size_t Length() const
        {
            return _Length;
        }

        constexpr inline void Length(size_t Length)
        {
            _Length = Length;
        }

        constexpr inline bool Growable() const
        {
            return _Growable;
        }

        constexpr inline void Growable(bool Enable)
        {
            _Growable = Enable;
        }

        constexpr inline T *Content()
        {
            return _Content.Content();
        }

        constexpr inline T const *Content() const
        {
            return _Content.Content();
        }

        constexpr inline bool IsEmpty() const noexcept { return _Length == 0; }

        constexpr inline bool IsFull() const noexcept { return _Length == Capacity(); }

        constexpr inline size_t IsFree() const noexcept { return Capacity() - _Length; }

        // Helper functions

        constexpr inline T &Head()
        {
            AssertNotEmpty();

            return _Content[0];
        }

        constexpr inline T const &Head() const
        {
            AssertNotEmpty();

            return _Content[0];
        }

        constexpr inline T &Tail()
        {
            AssertNotEmpty();

            return _ElementAt(_Length - 1);
        }

        constexpr inline T const &Tail() const
        {
            AssertNotEmpty();

            return _ElementAt(_Length - 1);
        }

        // Helper Functions

        constexpr void Resize(size_t Size)
        {
            // If size is the same and we can realign the content just do it withtout reallocating

            if (Size == Capacity())
                return;

            auto NewContent = Core::Iterable::MemoryHolder<T>(Size);

            // Copy old content to new buffer

            for (size_t i = 0; i < _Length; i++)
            {
                std::construct_at(&NewContent[i], std::move(_Content[i]));
            }

            _Content = std::move(NewContent);
        }

        constexpr void IncreaseCapacity(size_t Minimum = 1)
        {
            if (IsFree() >= Minimum)
                return;

            if (!_Growable)
                throw std::out_of_range("");

            Resize(_CalculateNewSize(Minimum - (Capacity() - _Length)));
        }

        constexpr inline void AdvanceTail(size_t Count = 1)
        {
            _Length += Count;
        }

        constexpr inline void RetrieveHead(size_t Count = 1)
        {
            _Length -= Count;
        }

        // Iteration functions

        template <class TCallback>
        constexpr void ForEach(TCallback Action)
        {
            for (size_t i = 0; i < _Length; i++)
            {
                Action(_Content[i]);
            }
        }

        template <class TCallback>
        constexpr void ForEach(TCallback Action) const
        {
            for (size_t i = 0; i < _Length; i++)
            {
                Action(_Content[i]);
            }
        }

        template <typename TCallback>
        constexpr std::optional<size_t> Contains(TCallback Condition) const
        {
            for (size_t i = 0; i < _Length; i++)
            {
                if (Condition(_Content[i]))
                    return i;
            }

            return std::nullopt;
        }

        // Adding functionality

        template <typename... TArgs>
        constexpr void Add(TArgs &&...Args)
        {
            IncreaseCapacity();

            std::construct_at(&_ElementAt(_Length++), std::forward<TArgs>(Args)...);
        }

        template <typename... TArgs>
        constexpr void Insert(T &&Item)
        {
            IncreaseCapacity();

            std::construct_at(&_ElementAt(_Length++), std::move(Item));
        }

        template <typename... TArgs>
        constexpr void Insert(T const &Item)
        {
            IncreaseCapacity();

            std::construct_at(&_ElementAt(_Length++), Item);
        }

        constexpr void MoveFrom(T *Data, size_t Count)
        {
            IncreaseCapacity(Count);

            auto Pointer = &_Content[_Length];

            for (size_t i = 0; i < Count; i++)
            {
                std::construct_at(&Pointer[i], std::move(Data[i]));
            }

            AdvanceTail(Count);
        }

        constexpr void CopyFrom(T const *Data, size_t Count)
        {
            IncreaseCapacity(Count);

            auto Pointer = &_Content[_Length];

            for (size_t i = 0; i < Count; i++)
            {
                std::construct_at(&Pointer[i], Data[i]);
            }

            AdvanceTail(Count);
        }

        // Take functionality

        constexpr T Take()
        {
            T Item = std::move(Head());

            RetrieveHead();

            return Item;
        }

        constexpr void MoveTo(T *Data, size_t Count)
        {
            if (Count > _Length)
                throw std::out_of_range("Take count exceeds the available data");

            auto LastIndex = _Length - 1;

            for (size_t i = 0; i < Count; i++)
            {
                Data[i] = std::move(_Content[LastIndex - i]);
            }

            RetrieveHead(Count);
        }

        constexpr void CopyTo(T *Data, size_t Count)
        {
            if (Count > _Length)
                throw std::out_of_range("Take count exceeds the available data");

            auto LastIndex = _Length - 1;

            for (size_t i = 0; i < Count; i++)
            {
                Data[i] = _Content[LastIndex - i];
            }

            // @todo What shold i do about this?

            // RetrieveHead(Count);
        }

        // Remove functionality

        constexpr void Pop()
        {
            std::destroy_at(&Tail());

            RetrieveHead();
        }

        constexpr void Free()
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                ForEach(
                    [](T &Item)
                    {
                        std::destroy_at(&Item);
                    });
            }

            this->_Length = 0;
        }

        // @todo Optimize this later

        constexpr void Free(size_t Count)
        {
            if (this->_Length < Count)
                throw std::out_of_range("");

            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                auto LastIndex = _Length - 1;

                for (size_t i = 0; i < Count; i++)
                {
                    std::destroy_at(&_Content[LastIndex - i]);
                }
            }

            this->_Length -= Count;
        }

    private:
        Core::Iterable::MemoryHolder<T, TAllocator> _Content;
        size_t _Length = 0;

        // @todo Seperate List from fixedList
        bool _Growable = true;

        constexpr inline void AssertNotEmpty()
        {
            if (IsEmpty())
                throw std::out_of_range("Instance is empty");
        }

        constexpr inline void AssertIndex(size_t Index)
        {
            if (Index >= _Length)
                throw std::out_of_range("Index out of range");
        }

        constexpr inline T &_ElementAt(size_t Index)
        {
            return this->_Content[Index];
        }

        constexpr inline const T &_ElementAt(size_t Index) const
        {
            return this->_Content[Index];
        }

        constexpr inline size_t _CalculateNewSize(size_t Minimum)
        {
            return (Capacity() * 2) + Minimum;
        }
    };
}