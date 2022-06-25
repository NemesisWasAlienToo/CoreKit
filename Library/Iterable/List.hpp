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
    class List final
    {
    public:
        // Constructors

        List() = default;
        List(size_t Size, bool Growable = true) : _Content(Size), _Length(0), _Growable(Growable) {}
        List(std::initializer_list<T> list) : _Content(list.size()), _Length(list.size()), _Growable(true)
        {
            for (auto &Item : list)
                Add(Item);
        }

        List(List const &Other) : _Content(Other._Content), _Length(Other._Length), _Growable(Other._Growable)
        {
            for (size_t i = 0; i < _Length; i++)
            {
                std::construct_at(&_Content[i], Other._Content[i]);
            }
        }

        List(List &&Other) : _Content(std::move(Other._Content)), _Length(Other._Length), _Growable(Other._Growable)
        {
            Other._Length = 0;
            Other._Growable = true;
        }

        ~List()
        {
            Free();
        }

        // Operators

        List &operator=(List const &Other)
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

        List &operator=(List &&Other)
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

        T &operator[](size_t Index)
        {
            if (Index >= _Length)
                throw std::out_of_range("Index out of range");

            return _Content[Index];
        }

        T const &operator[](size_t Index) const
        {
            if (Index >= _Length)
                throw std::out_of_range("Index out of range");

            return _Content[Index];
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

        inline void Length(size_t Length)
        {
            _Length = Length;
        }

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

        inline bool IsEmpty() const noexcept { return _Length == 0; }

        inline bool IsFull() const noexcept { return _Length == Capacity(); }

        inline size_t IsFree() const noexcept { return Capacity() - _Length; }

        // Helper functions

        inline T &Head()
        {
            AssertNotEmpty();

            return _Content[0];
        }

        inline T const &Head() const
        {
            AssertNotEmpty();

            return _Content[0];
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

        // std::tuple<T *, size_t> DataChunk(size_t Start = 0)
        // {
        //     return std::make_tuple(&_ElementAt(Start), std::min((Capacity() - ((_First + Start) % Capacity())), _Length - Start));
        // }

        // std::tuple<T const *, size_t> DataChunk(size_t Start = 0) const
        // {
        //     return std::make_tuple(&_ElementAt(Start), std::min((Capacity() - ((_First + Start) % Capacity())), _Length - Start));
        // }

        // std::tuple<T *, size_t> EmptyChunk(size_t Start = 0)
        // {
        //     size_t FirstEmpty = (_First + _Length + Start) % Capacity();

        //     return std::make_tuple(&_Content[FirstEmpty], _First <= FirstEmpty ? Capacity() - (FirstEmpty) : _First - FirstEmpty);
        // }

        // std::tuple<T const *, size_t> EmptyChunk(size_t Start = 0) const
        // {
        //     size_t FirstEmpty = (_First + _Length + Start) % Capacity();

        //     return std::make_tuple(&_Content[FirstEmpty], _First <= FirstEmpty ? Capacity() - (FirstEmpty) : _First - FirstEmpty);
        // }

        // bool DataVector(struct iovec *Vector)
        // {
        //     auto [FPointer, FSize] = DataChunk();

        //     Vector[0].iov_base = reinterpret_cast<void *>(FPointer);
        //     Vector[0].iov_len = FSize;

        //     if (IsWrapped())
        //     {
        //         auto [SPointer, SSize] = DataChunk(FSize);

        //         Vector[1].iov_base = reinterpret_cast<void *>(SPointer);
        //         Vector[1].iov_len = SSize;

        //         return true;
        //     }

        //     return false;
        // }

        // bool EmptyVector(struct iovec *Vector)
        // {
        //     auto [FPointer, FSize] = EmptyChunk();

        //     Vector[0].iov_base = reinterpret_cast<void *>(FPointer);
        //     Vector[0].iov_len = FSize;

        //     if (!this->IsEmpty() && this->_First != 0 && this->_First + this->_Length != this->Capacity())
        //     {
        //         auto [SPointer, SSize] = EmptyChunk(FSize);

        //         Vector[1].iov_base = reinterpret_cast<void *>(SPointer);
        //         Vector[1].iov_len = SSize;

        //         return true;
        //     }

        //     return false;
        // }

        // Helper Functions

        void Resize(size_t Size)
        {
            // If size is the same and we can realign the content just do it withtout reallocating

            if (Size == Capacity())
                return;

            auto NewContent = MemoryHolder<T>(Size);

            // Copy old content to new buffer

            for (size_t i = 0; i < _Length; i++)
            {
                std::construct_at(&NewContent[i], std::move(_Content[i]));
            }

            _Content = std::move(NewContent);
        }

        void IncreaseCapacity(size_t Minimum = 1)
        {
            if (IsFree() >= Minimum)
                return;

            if (!_Growable)
                throw std::out_of_range("");

            Resize(_CalculateNewSize(Minimum - (Capacity() - _Length)));
        }

        inline void AdvanceTail(size_t Count = 1)
        {
            _Length += Count;
        }

        inline void RetrieveHead(size_t Count = 1)
        {
            _Length -= Count;
        }

        // Iteration functions

        template <class TCallback>
        void ForEach(TCallback Action)
        {
            for (size_t i = 0; i < _Length; i++)
            {
                Action(_Content[i]);
            }
        }

        template <class TCallback>
        void ForEach(TCallback Action) const
        {
            for (size_t i = 0; i < _Length; i++)
            {
                Action(_Content[i]);
            }
        }

        template <typename TCallback>
        std::optional<size_t> Contains(TCallback Condition) const
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
        void Add(TArgs &&...Args)
        {
            IncreaseCapacity();

            std::construct_at(&_ElementAt(_Length++), std::forward<TArgs>(Args)...);
        }

        void MoveFrom(T *Data, size_t Count)
        {
            IncreaseCapacity(Count);

            auto Pointer = &_Content[_Length];

            for (size_t i = 0; i < Count; i++)
            {
                std::construct_at(&Pointer[i], std::move(Data[i]));
            }

            AdvanceTail(Count);
        }

        void CopyFrom(T const *Data, size_t Count)
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

        T Take()
        {
            T Item = std::move(Head());

            RetrieveHead();

            return Item;
        }

        void MoveTo(T *Data, size_t Count)
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

        void CopyTo(T *Data, size_t Count)
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

        void Pop()
        {
            std::destroy_at(&Tail());

            RetrieveHead();
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
        }

        // @todo Optimize this later

        void Free(size_t Count)
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
            return this->_Content[Index];
        }

        inline const T &_ElementAt(size_t Index) const
        {
            return this->_Content[Index];
        }

        inline size_t _CalculateNewSize(size_t Minimum)
        {
            return (Capacity() * 2) + Minimum;
        }
    };
}