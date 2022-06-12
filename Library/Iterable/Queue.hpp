#pragma once

#include <sys/uio.h>
#include "Iterable/Span.hpp"
#include "Iterable/Iterable.hpp"

namespace Core
{
    namespace Iterable
    {
        template <typename T>
        class Queue : public Iterable<T>
        {

        protected:
            // ### Private variables

            size_t _First = 0;

            // ### Private Functions

            inline T &_ElementAt(size_t Index) override
            {
                return this->_Content[(_First + Index) % this->_Capacity];
            }

            inline const T &_ElementAt(size_t Index) const override
            {
                return this->_Content[(_First + Index) % this->_Capacity];
            }

        public:
            // ### Constructors

            Queue() = default;

            Queue(size_t Capacity, bool Growable = true) : Iterable<T>(Capacity, Growable), _First(0) {}

            Queue(T *Array, int Count, bool Growable = true) : Iterable<T>(Array, Count, Growable), _First(0) {}

            Queue(const Queue &Other) : Iterable<T>(Other), _First(0) {}

            Queue(Queue &&Other) noexcept : Iterable<T>(std::move(Other)), _First(Other._First)
            {
                Other._First = 0;
            }

            Queue(std::initializer_list<T> list) : Iterable<T>(list), _First(0) {}

            // ### Destructor

            ~Queue() = default;

            // ### Properties

            std::tuple<const T *, size_t> Chunk(size_t Start = 0) const
            {
                return std::make_tuple(&_ElementAt(Start), std::min((this->_Capacity - (this->_First + Start)), this->_Length - Start));
            }

            // @todo Unit test these

            std::tuple<T *, size_t> Chunk(size_t Start = 0)
            {
                return std::make_tuple(&_ElementAt(Start), std::min((this->_Capacity - (this->_First + Start)), this->_Length - Start));
            }

            std::tuple<T *, size_t> EmptyChunk(size_t Start = 0)
            {
                size_t FirstEmpty = (_First + this->_Length + Start) % this->_Capacity;

                return std::make_tuple(this->_Content + FirstEmpty, _First <= FirstEmpty ? this->_Capacity - (FirstEmpty) : FirstEmpty - this->_First);
            }

            bool DataVector(struct iovec *Vector)
            {
                auto [FPointer, FSize] = Chunk();

                Vector[0].iov_base = reinterpret_cast<void *>(FPointer);
                Vector[0].iov_len = FSize;

                if (IsWrapped())
                {
                    auto [SPointer, SSize] = Chunk(FSize);

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

                if (!this->IsEmpty() && this->_First != 0 && this->_First + this->_Length != this->_Capacity)
                {
                    auto [SPointer, SSize] = EmptyChunk(FSize);

                    Vector[1].iov_base = reinterpret_cast<void *>(SPointer);
                    Vector[1].iov_len = SSize;

                    return true;
                }

                return false;
            }

            template<typename ...TArgs>
            void Construct(TArgs &&...Args)
            {
                if (this->_Length == this->_Capacity)
                    this->_IncreaseCapacity();

                std::construct_at(&_ElementAt(this->_Length), std::forward<TArgs>(Args)...);
                this->_Length++;
            }

            void Add(T &&Item)
            {
                this->_IncreaseCapacity();

                _ElementAt(this->_Length) = std::move(Item);
                (this->_Length)++;
            }

            void Add(const T &Item)
            {
                this->_IncreaseCapacity();

                _ElementAt(this->_Length) = Item;
                (this->_Length)++;
            }

            void Add(T *Items, size_t Count)
            {
                this->_IncreaseCapacity(Count);

                size_t Index = 0;

                while (Index < Count)
                {
                    auto [Pointer, Size] = EmptyChunk();

                    Size = std::min(Size, Count);

                    for (size_t i = 0; i < Size; i++)
                    {
                        Pointer[i] = std::move(Items[Index++]);
                    }
                }

                this->_Length += Count;
            }

            void Add(const T *Items, size_t Count)
            {
                this->_IncreaseCapacity(Count);

                size_t Index = 0;

                while (Index < Count)
                {
                    auto [Pointer, Size] = EmptyChunk();

                    Size = std::min(Size, Count);

                    for (size_t i = 0; i < Size; i++)
                    {
                        Pointer[i] = Items[Index++];
                    }
                }

                this->_Length += Count;
            }

            inline T &First()
            {
                return _ElementAt(0);
            }

            inline T const &First() const
            {
                return _ElementAt(0);
            }

            // ### Public Functions

            inline bool IsWrapped() const
            {
                return this->_First + this->_Length > this->_Capacity;
            }

            void Resize(size_t Size) override
            {
                // Only if the capacity is the same and buffer did not wrap around

                if (Size == this->_Length && !IsWrapped())
                {
                    for (size_t i = 0; i < this->_Length; i++)
                    {
                        // Because it will not wrap around we can ignore modulo indexing

                        this->_Content[i] = std::move(this->_Content[this->_First + i]);
                    }
                }
                else
                {
                    Iterable<T>::Resize(Size);
                }

                _First = 0;
            }

            // due to a bug in gcc, if optimization flags are present
            // this function will act in an undefined manner

            // void Pop()
            // {
            //     if (this->IsEmpty())
            //         throw std::out_of_range("");

            //     std::destroy_at(&First());

            //     _First = (_First + 1) % this->_Capacity;
            //     this->_Length--;
            // }

            void Take(T &Item)
            {
                if (this->IsEmpty())
                    throw std::out_of_range("");

                Item = std::move(_ElementAt(0)); // OK?
                this->_Length--;
                _First = (_First + 1) % this->_Capacity;
            }

            // @todo Optimize this

            T Take()
            {
                T Item;
                Take(Item);
                return Item;
            }

            void Take(T *Items, size_t Count)
            {
                if (this->_Length < Count)
                    throw std::out_of_range("");

                for (size_t i = 0; i < Count; i++)
                {
                    Items[i] = std::move(_ElementAt(i));
                }

                _First = (_First + Count) % this->_Capacity;
                this->_Length -= Count;
            }

            void Free()
            {
                if constexpr (!std::is_arithmetic<T>::value)
                {
                    for (size_t i = 0; i < this->_Length; i++)
                    {
                        _ElementAt(i).~T();
                    }
                }

                this->_Length = 0;
                this->_First = 0;
            }

            void Free(size_t Count)
            {
                if (this->_Length < Count)
                    throw std::out_of_range("");

                if constexpr (!std::is_arithmetic<T>::value)
                {
                    for (size_t i = 0; i < Count; i++)
                    {
                        _ElementAt(i).~T();
                    }
                }

                this->_Length -= Count;

                if (this->_Length == 0)
                {
                    this->_First = 0;
                }
                else
                {
                    _First = (_First + Count) % this->_Capacity;
                }
            }

            void Rewind(size_t Steps)
            {
                if constexpr (std::is_integral_v<T>)
                {
                    throw std::runtime_error("You cannot rewind an unitegral queue.");
                }

                if (this->_Capacity < Steps)
                    throw std::out_of_range("");

                _First = (this->_Capacity - Steps + _First) % this->_Capacity;

                this->_Length += Steps;
            }

            // ### Operators

            Queue &operator=(const Queue &Other) = default;

            Queue &operator=(Queue &&Other) noexcept = default;

            T &operator[](const size_t &Index)
            {
                if (Index >= this->_Length)
                    throw std::out_of_range("");

                return _ElementAt(Index);
            }

            Queue &operator>>(T &Item)
            {

                Item = Take();

                return *this;
            }
        };
    }
}