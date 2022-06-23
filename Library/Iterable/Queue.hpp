#pragma once

#include <tuple>
#include <memory>
#include <initializer_list>
#include <Iterable/Span.hpp>
#include <sys/uio.h>

namespace Core::Iterable
{
    template <typename T>
    class Queue final
    {
    public:
        // Constructors

        Queue() = default;
        Queue(size_t Size, bool Growable = true) : _Content(Size), _First(0), _Length(0), _Growable(Growable) {}
        Queue(std::initializer_list<T> list) : _Content(list), _First(0), _Length(list.size()), _Growable(true) {}

        Queue(Queue const &Other) : _Content(Other._Content), _First(Other._First), _Length(Other._Length), _Growable(Other._Growable) {}
        Queue(Queue &&Other) : _Content(std::move(Other._Content)), _First(Other._First), _Length(Other._Length), _Growable(Other._Growable)
        {
            Other._First = 0;
            Other._Length = 0;
            Other._Growable = true;
        }

        // Operators

        Queue &operator=(Queue const &Other)
        {
            if (this != &Other)
            {
                _Content = Other._Content;
                _First = Other._First;
                _Length = Other._Length;
                _Growable = Other._Growable;
            }

            return *this;
        }

        Queue &operator=(Queue &&Other)
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

        T &operator[](size_t Index)
        {
            if (Index >= _Length)
                throw std::out_of_range("Index out of range");

            return _Content.Content()[(_First + Index) % Capacity()];
        }

        T const &operator[](size_t Index) const
        {
            if (Index >= _Length)
                throw std::out_of_range("Index out of range");

            return _Content.Content()[(_First + Index) % Capacity()];
        }

        // Peroperties

        inline size_t Capacity() const
        {
            return _Content.Length();
        }

        inline size_t Length() const
        {
            return _Length;
        }

        // inline void Length(size_t Lenght)
        // {
        //     _Length = Lenght;
        // }

        inline bool Growable() const
        {
            return _Growable;
        }

        inline void Growable(bool Enable)
        {
            _Growable = Enable;
        }

        inline T *Content()
        {
            return _Content.Content();
        }

        inline T const *Content() const
        {
            return _Content.Content();
        }

        inline bool IsWrapped() const
        {
            return _First + _Length > Capacity();
        }

        inline bool IsEmpty() noexcept { return _Length == 0; }

        inline bool IsFull() noexcept { return _Length == Capacity(); }

        inline size_t IsFree() noexcept { return Capacity() - _Length; }

        // Helper functions

        inline T &Head()
        {
            AssertNotEmpty();

            return _Content.Content()[_First];
        }

        inline T const &Head() const
        {
            AssertNotEmpty();

            return _Content.Content()[_First];
        }

        inline T &Tail()
        {
            AssertNotEmpty();

            return _ElementAt(_Length - 1);
        }

        inline T const &Tail() const
        {
            AssertNotEmpty();

            return _ElementAt(_Length - 1);
        }

        std::tuple<T *, size_t> DataChunk(size_t Start = 0)
        {
            return std::make_tuple(&_ElementAt(Start), std::min((Capacity() - ((_First + Start) % Capacity())), _Length - Start));
        }

        std::tuple<T *, size_t> EmptyChunk(size_t Start = 0)
        {
            size_t FirstEmpty = (_First + _Length + Start) % Capacity();

            return std::make_tuple(_Content.Content() + FirstEmpty, _First <= FirstEmpty ? Capacity() - (FirstEmpty) : FirstEmpty - _First);
        }

        bool DataVector(struct iovec *Vector)
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

        bool EmptyVector(struct iovec *Vector)
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

        void Resize(size_t Size)
        {
            // If size is the same and we can realign the content just do it withtout reallocating

            if (Size == Capacity() && Size == this->_Length && !IsWrapped())
            {
                for (size_t i = 0; i < this->_Length; i++)
                {
                    // Because it will not wrap around we can ignore modulo indexing

                    this->_Content[i] = std::move(this->_Content[this->_First + i]);
                }
            }
            else
            {
                // Allocate new buffer

                auto NewContent = Span<T>(Size);

                // Copy old content to new buffer

                size_t Index = 0;
                size_t Size = 0;

                while (Index < _Length)
                {
                    auto [Pointer, _Size] = DataChunk(Size);

                    Size = std::min(_Size, _Length);

                    for (size_t i = 0; i < Size; i++)
                    {
                        NewContent[Index++] = std::move(Pointer[i]);
                    }
                }

                _Content = std::move(NewContent);
            }

            _First = 0;
        }

        void IncreaseCapacity(size_t Minimum = 1)
        {
            if (IsFree() >= Minimum)
                return;

            if (!_Growable)
                throw std::out_of_range("");

            Resize(_CalculateNewSize(Minimum - (Capacity() - _Length)));
        }

        inline void AdvanceHead(size_t Count = 1)
        {
            _Length -= Count;
            _First = (_First + Count) % Capacity();
        }

        inline void AdvanceTail(size_t Count = 1)
        {
            _Length += Count;
        }

        // Iteration functions

        template <class TCallback>
        void ForEach(TCallback Action)
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
        void ForEach(TCallback Action) const
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
        std::optional<size_t> Contains(TCallback Callback) const
        {
            size_t Index = 0;

            while (Index < _Length)
            {
                auto [Pointer, Size] = DataChunk();

                for (size_t i = 0; i < Size; i++)
                {
                    if (Callback(Pointer[i]))
                        return i;
                }

                Index += Size;
            }

            return std::nullopt;
        }

        // Adding functionality

        template <typename... TArgs>
        void Add(TArgs &&...Args)
        {
            IncreaseCapacity();

            std::construct_at(&_ElementAt(_Length++), std::forward<TArgs>(Args)...);
        }

        template <class TCallback>
        void AddRange(size_t Count, TCallback Action)
        {
            IncreaseCapacity(Count);

            size_t Index = 0;

            while (Index < Count)
            {
                auto [Pointer, Size] = EmptyChunk(Index);

                for (size_t i = 0; i < Size && Index < Count; i++)
                {
                    Action(Pointer[i], Index++);
                }
            }

            AdvanceTail(Count);
        }

        void MoveFrom(T *Data, size_t Count)
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

        void CopyFrom(T const *Data, size_t Count)
        {
            IncreaseCapacity(Count);

            size_t Index = 0;

            while (Index < Count)
            {
                auto [Pointer, Size] = EmptyChunk(Index);

                for (size_t i = 0; i < Size && Index < Count; i++)
                {
                    Pointer[i] = Data[Index++];
                }
            }

            AdvanceTail(Count);
        }

        // Take functionality

        T Take()
        {
            T Item = std::move(Head());

            AdvanceHead();

            return Item;
        }

        template <typename TCallback>
        void TakeRange(size_t Count, TCallback Callback)
        {
            if (Count > IsFree())
                throw std::out_of_range("Take count exceeds the available data");

            size_t Index = 0;

            while (Index < Count)
            {
                auto [Pointer, Size] = DataChunk(Index);

                for (size_t i = 0; i < Size && Index < Count; i++)
                {
                    Callback(Pointer[i], Index++);
                }
            }

            AdvanceHead(Count);
        }

        void MoveTo(T *Data, size_t Count)
        {
            if (Count > IsFree())
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

        void CopyTo(T *Data, size_t Count)
        {
            if (Count > IsFree())
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

        void Pop()
        {
            std::destroy_at(std::addressof(Head()));

            AdvanceHead();
        }

        void Free()
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

        void Free(size_t Count)
        {
            if (this->_Length < Count)
                throw std::out_of_range("");

            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                // @todo Optimize this

                for (size_t i = 0; i < Count; i++)
                {
                    std::destroy_at(&_ElementAt(i));
                }
            }

            this->_Length -= Count;

            this->_First = this->_Length == 0 ? 0 : (_First + Count) % this->Capacity();
        }

    private:
        Core::Iterable::Span<T> _Content;
        size_t _First = 0;
        size_t _Length = 0;
        bool _Growable = true;

        inline void AssertNotEmpty()
        {
            if (IsEmpty())
                throw std::out_of_range("Instance is empty");
        }

        inline void AssertIndex(size_t Index)
        {
            if (Index >= _Length)
                throw std::out_of_range("Index out of range");
        }

        inline T &_ElementAt(size_t Index)
        {
            return this->_Content[(_First + Index) % Capacity()];
        }

        inline const T &_ElementAt(size_t Index) const
        {
            return this->_Content[(_First + Index) % Capacity()];
        }

        inline size_t _CalculateNewSize(size_t Minimum)
        {
            return (Capacity() * 2) + Minimum;
        }
    };
}