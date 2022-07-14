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
        constexpr MemoryHolder() = default;
        constexpr MemoryHolder(size_t Size) : _Content(Allocate(Size)), _Length(Size) {}
        constexpr MemoryHolder(MemoryHolder const &Other) : _Content(Allocate(Other._Length)), _Length(Other._Length) {}
        constexpr MemoryHolder(MemoryHolder &&Other) : _Content(Other._Content), _Length(Other._Length)
        {
            Other._Content = nullptr;
            Other._Length = 0;
        }

        constexpr ~MemoryHolder()
        {
            Deallocate();
        }

        constexpr MemoryHolder &operator=(MemoryHolder &&Other)
        {
            Deallocate();

            _Content = Other._Content;
            _Length = Other._Length;

            Other._Content = nullptr;
            Other._Length = 0;

            return *this;
        }

        constexpr MemoryHolder &operator=(MemoryHolder const &Other)
        {
            Deallocate();

            _Content = Allocate(Other._Length);
            _Length = Other._Length;

            return *this;
        }

        constexpr inline T *Allocate(size_t Count)
        {
            return _Allocator.allocate(Count);
        }

        constexpr inline void Deallocate()
        {
            if (_Content)
                _Allocator.deallocate(_Content, _Length);
        }

        constexpr inline size_t Length() const { return _Length; }

        constexpr inline T *Content() { return _Content; }
        constexpr inline T const *Content() const { return _Content; }

        constexpr inline T &operator[](size_t Index) { return _Content[Index]; }
        constexpr inline T const &operator[](size_t Index) const { return _Content[Index]; }
    };
}