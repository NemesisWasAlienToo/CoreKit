#pragma once

#include <memory>

namespace Core::Iterable
{
    template <typename T, typename TAllocator = std::allocator<T>>
    class MemoryHolder
    {
    private:
        TAllocator _Allocator;
        T *_Content = nullptr;
        size_t _Length = 0;

    public:
        MemoryHolder() = default;
        MemoryHolder(size_t Size) : _Content(Allocate(Size)), _Length(Size) {}
        MemoryHolder(MemoryHolder const &Other) : _Content(Allocate(Other._Length)), _Length(Other._Length) {}
        MemoryHolder(MemoryHolder &&Other) : _Content(Other._Content), _Length(Other._Length)
        {
            Other._Content = nullptr;
            Other._Length = 0;
        }

        ~MemoryHolder()
        {
            Deallocate();
        }

        MemoryHolder &operator=(MemoryHolder &&Other)
        {
            Deallocate();

            _Content = Other._Content;
            _Length = Other._Length;

            Other._Content = nullptr;
            Other._Length = 0;

            return *this;
        }

        MemoryHolder &operator=(MemoryHolder const &Other)
        {
            Deallocate();

            _Content = Allocate(Other._Length);
            _Length = Other._Length;

            return *this;
        }

        inline T *Allocate(size_t Count)
        {
            return _Allocator.allocate(Count);
        }

        inline void Deallocate()
        {
            _Allocator.deallocate(_Content, _Length);
        }

        inline size_t Length() const { return _Length; }

        inline T *Content() { return _Content; }
        inline T const *Content() const { return _Content; }

        inline T &operator[](size_t Index) { return _Content[Index]; }
        inline T const &operator[](size_t Index) const { return _Content[Index]; }
    };
}