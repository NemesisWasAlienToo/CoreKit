#pragma once

#include <tuple>
#include <memory>
#include <optional>
#include <initializer_list>
#include <sys/uio.h>

#include <Iterable/MemoryHolder.hpp>

namespace Core::Iterable
{
    template <typename T, typename TAllocator = std::allocator<T>>
    class Queue final
    {
    public:
        // Constructors

        constexpr Queue() = default;
        constexpr Queue(size_t Size, bool Growable = true) : _Content(Size), _First(0), _Length(0), _Growable(Growable) {}
        constexpr Queue(std::initializer_list<T> list) : _Content(list.size()), _First(0), _Length(0), _Growable(true)
        {
            for (auto &Item : list)
                Add(Item);
        }

        constexpr Queue(Queue const &Other) : _Content(Other._Content), _First(0), _Length(Other._Length), _Growable(Other._Growable)
        {
            size_t Index = 0;

            while (Index < _Length)
            {
                auto [Pointer, Size] = Other.DataChunk(Index);

                for (size_t i = 0; i < Size; i++)
                {
                    std::construct_at(&_Content[Index++], Pointer[i]);
                }
            }
        }

        constexpr Queue(Queue &&Other) : _Content(std::move(Other._Content)), _First(Other._First), _Length(Other._Length), _Growable(Other._Growable)
        {
            Other._First = 0;
            Other._Length = 0;
            Other._Growable = true;
        }

        constexpr ~Queue()
        {
            Free();
        }

        // Operators

        constexpr Queue &operator=(Queue const &Other)
        {
            if (this != &Other)
            {
                _Content = Other._Content;
                _First = 0;
                _Length = Other._Length;
                _Growable = Other._Growable;

                size_t Index = 0;

                while (Index < _Length)
                {
                    auto [Pointer, Size] = Other.DataChunk(Index);

                    for (size_t i = 0; i < Size; i++)
                    {
                        _Content[Index++] = Pointer[i];
                    }
                }
            }

            return *this;
        }

        constexpr Queue &operator=(Queue &&Other)
        {
            if (this != &Other)
            {
                _Content = std::move(Other._Content);
                _First = Other._First;
                _Length = Other._Length;
                _Growable = Other._Growable;

                Other._First = 0;
                Other._Length = 0;
                Other._Growable = true;
            }

            return *this;
        }

        constexpr T &operator[](size_t Index)
        {
            if (Index >= _Length)
                throw std::out_of_range("Index out of range");

            return _Content[(_First + Index) % Capacity()];
        }

        constexpr T const &operator[](size_t Index) const
        {
            if (Index >= _Length)
                throw std::out_of_range("Index out of range");

            return _Content[(_First + Index) % Capacity()];
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

        constexpr inline bool IsWrapped() const
        {
            return _First + _Length > Capacity();
        }

        constexpr inline bool IsEmpty() const noexcept { return _Length == 0; }

        constexpr inline bool IsFull() const noexcept { return _Length == Capacity(); }

        constexpr inline size_t IsFree() const noexcept { return Capacity() - _Length; }

        // Helper functions

        constexpr inline T &Head()
        {
            AssertNotEmpty();

            return _Content[_First];
        }

        constexpr inline T const &Head() const
        {
            AssertNotEmpty();

            return _Content[_First];
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

        constexpr std::tuple<T *, size_t> DataChunk(size_t Start = 0)
        {
            return std::make_tuple(&_ElementAt(Start), std::min((Capacity() - ((_First + Start) % Capacity())), _Length - Start));
        }

        constexpr std::tuple<T const *, size_t> DataChunk(size_t Start = 0) const
        {
            return std::make_tuple(&_ElementAt(Start), std::min((Capacity() - ((_First + Start) % Capacity())), _Length - Start));
        }

        constexpr std::tuple<T *, size_t> EmptyChunk(size_t Start = 0)
        {
            size_t FirstEmpty = (_First + _Length + Start) % Capacity();

            return std::make_tuple(&_Content[FirstEmpty], _First <= FirstEmpty ? Capacity() - (FirstEmpty) : _First - FirstEmpty);
        }

        constexpr std::tuple<T const *, size_t> EmptyChunk(size_t Start = 0) const
        {
            size_t FirstEmpty = (_First + _Length + Start) % Capacity();

            return std::make_tuple(&_Content[FirstEmpty], _First <= FirstEmpty ? Capacity() - (FirstEmpty) : _First - FirstEmpty);
        }

        constexpr bool DataVectors(struct iovec *Vector)
        {
            auto [FPointer, FSize] = DataChunk();

            Vector[0].iov_base = reinterpret_cast<void *>(FPointer);
            Vector[0].iov_len = FSize;

            if (IsWrapped())
            {
                auto [SPointer, SSize] = DataChunk(FSize);

                Vector[1].iov_base = reinterpret_cast<void *>(SPointer);
                Vector[1].iov_len = SSize;

                return true;
            }

            return false;
        }

        constexpr bool EmptyVectors(struct iovec *Vector)
        {
            auto [FPointer, FSize] = EmptyChunk();

            Vector[0].iov_base = reinterpret_cast<void *>(FPointer);
            Vector[0].iov_len = FSize;

            if (!this->IsEmpty() && this->_First != 0 && this->_First + this->_Length != this->Capacity())
            {
                auto [SPointer, SSize] = EmptyChunk(FSize);

                Vector[1].iov_base = reinterpret_cast<void *>(SPointer);
                Vector[1].iov_len = SSize;

                return true;
            }

            return false;
        }

        // Helper Functions

        constexpr bool Realign()
        {
            if (IsWrapped())
                return false;

            for (size_t i = 0; i < this->_Length; i++)
            {
                // Because it will not wrap around we can ignore modulo indexing

                this->_Content[i] = std::move(this->_Content[this->_First + i]);
            }

            _First = 0;

            return true;
        }

        constexpr void Resize(size_t Size)
        {
            // If size is the same and we can realign the content just do it withtout reallocating

            if (Size == Capacity() && Realign())
                return;

            auto NewContent = MemoryHolder<T>(Size);

            // Copy old content to new buffer

            size_t Index = 0;

            while (Index < _Length)
            {
                auto [Pointer, _Size] = DataChunk(Index);

                for (size_t i = 0; i < _Size; i++)
                {
                    std::construct_at(&NewContent[Index++], std::move(Pointer[i]));
                }
            }

            _Content = std::move(NewContent);
            _First = 0;
        }

        constexpr void IncreaseCapacity(size_t Minimum = 1)
        {
            if (IsFree() >= Minimum)
                return;

            if (!_Growable)
                throw std::out_of_range("");

            Resize(_CalculateNewSize(Minimum - (Capacity() - _Length)));
        }

        constexpr inline void AdvanceHead(size_t Count = 1)
        {
            _Length -= Count;
            _First = (_First + Count) % Capacity();
        }

        constexpr inline void AdvanceTail(size_t Count = 1)
        {
            _Length += Count;
        }

        // Iteration functions

        template <class TCallback>
        constexpr void For(TCallback Action)
        {
            size_t Index = 0;

            while (Index < _Length)
            {
                auto [Pointer, Size] = DataChunk(Index);

                for (size_t i = 0; i < Size; i++)
                {
                    Action(Pointer[i], Index++);
                }
            }
        }

        template <class TCallback>
        constexpr void For(TCallback Action) const
        {
            size_t Index = 0;

            while (Index < _Length)
            {
                auto [Pointer, Size] = DataChunk(Index);

                for (size_t i = 0; i < Size; i++)
                {
                    Action(Pointer[i], Index++);
                }
            }
        }

        template <class TCallback>
        constexpr void ForEach(TCallback Action)
        {
            size_t Index = 0;

            while (Index < _Length)
            {
                auto [Pointer, Size] = DataChunk(Index);

                for (size_t i = 0; i < Size; i++)
                {
                    Action(Pointer[i]);
                }

                Index += Size;
            }
        }

        template <class TCallback>
        constexpr void ForEach(TCallback Action) const
        {
            size_t Index = 0;

            while (Index < _Length)
            {
                auto [Pointer, Size] = DataChunk(Index);

                for (size_t i = 0; i < Size; i++)
                {
                    Action(Pointer[i]);
                }

                Index += Size;
            }
        }

        template <typename TCallback>
        constexpr std::optional<size_t> Contains(TCallback Condition) const
        {
            size_t Index = 0;

            while (Index < _Length)
            {
                auto [Pointer, Size] = DataChunk(Index);

                for (size_t i = 0; i < Size; i++)
                {
                    if (Condition(Pointer[i]))
                        return i;
                }

                Index += Size;
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

        constexpr void Insert(T &&Item)
        {
            IncreaseCapacity();

            std::construct_at(&_ElementAt(_Length++), std::move(Item));
        }

        constexpr void Insert(T const &Item)
        {
            IncreaseCapacity();

            std::construct_at(&_ElementAt(_Length++), Item);
        }

        constexpr void MoveFrom(T *Data, size_t Count)
        {
            IncreaseCapacity(Count);

            size_t Index = 0;

            while (Index < Count)
            {
                auto [Pointer, Size] = EmptyChunk(Index);

                for (size_t i = 0; i < Size && Index < Count; i++)
                {
                    std::construct_at(&Pointer[i], std::move(Data[Index++]));
                }
            }

            AdvanceTail(Count);
        }

        constexpr void CopyFrom(T const *Data, size_t Count)
        {
            IncreaseCapacity(Count);

            size_t Index = 0;

            while (Index < Count)
            {
                auto [Pointer, Size] = EmptyChunk(Index);

                for (size_t i = 0; i < Size && Index < Count; i++)
                {
                    std::construct_at(&Pointer[i], Data[Index++]);
                }
            }

            AdvanceTail(Count);
        }

        // Take functionality

        constexpr T Take()
        {
            T Item = std::move(Head());

            AdvanceHead();

            return Item;
        }

        constexpr void MoveTo(T *Data, size_t Count)
        {
            if (Count > _Length)
                throw std::out_of_range("Take count exceeds the available data");

            size_t Index = 0;

            while (Index < Count)
            {
                auto [Pointer, Size] = DataChunk(Index);

                for (size_t i = 0; i < Size && Index < Count; i++)
                {
                    Data[Index++] = std::move(Pointer[i]);
                }
            }

            AdvanceHead(Count);
        }

        constexpr void CopyTo(T *Data, size_t Count)
        {
            if (Count > _Length)
                throw std::out_of_range("Take count exceeds the available data");

            size_t Index = 0;

            while (Index < Count)
            {
                auto [Pointer, Size] = DataChunk(Index);

                for (size_t i = 0; i < Size && Index < Count; i++)
                {
                    Data[Index++] = Pointer[i];
                }
            }

            // @todo What shold i do about this?

            // AdvanceHead(Count);
        }

        // Remove functionality

        constexpr void Pop()
        {
            std::destroy_at(&Head());

            AdvanceHead();
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
            this->_First = 0;
        }

        // @todo Optimize this later

        constexpr void Free(size_t Count)
        {
            if (this->_Length < Count)
                throw std::out_of_range("");

            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                size_t Index = 0;

                while (Index < Count)
                {
                    auto [Pointer, Size] = DataChunk(Index);

                    for (size_t i = 0; i < Size && Index < Count; i++, Index++)
                    {
                        std::destroy_at(&Pointer[i]);
                    }
                }
            }

            this->_Length -= Count;

            this->_First = this->_Length == 0 ? 0 : (_First + Count) % this->Capacity();
        }

    private:
        Core::Iterable::MemoryHolder<T, TAllocator> _Content;
        size_t _First = 0;
        size_t _Length = 0;

        // @todo Seperate Queue from fixedQueue
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
            return this->_Content[(_First + Index) % Capacity()];
        }

        constexpr inline const T &_ElementAt(size_t Index) const
        {
            return this->_Content[(_First + Index) % Capacity()];
        }

        constexpr inline size_t _CalculateNewSize(size_t Minimum)
        {
            return (Capacity() * 2) + Minimum;
        }
    };
}