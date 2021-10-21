namespace Core
{
    template <typename T>
    class Sequence
    {
    protected:
        static_assert(std::is_move_assignable<T>::value, "T must be move assignable");
        static_assert(std::is_copy_assignable<T>::value, "T must be copy assignable");

        size_t _Capacity = 0;
        size_t _Length = 0;
        T *_Content = NULL;

    public:
        Sequence(size_t Capacity) : _Capacity(Capacity), _Length(0), _Content(new T[Capacity]) {}

        Sequence(Sequence &Other) : _Capacity(Other._Capacity), _Length(Other._Length), _Content(new T[Other._Capacity])
        {
            for (size_t i = 0; i < Other._Length; i++)
            {
                _Content[i] = Other._Content[(Other._First + i) % Other._Capacity];
            }
        }

        Sequence(Sequence &&Other) noexcept : _Capacity(Other._Capacity), _Length(Other._Length)
        {
            std::swap(_Content, Other._Content);
        }

        inline T *Content()
        {
            return _Content;
        }

        inline size_t Length()
        {
            return _Length;
        }

        inline size_t Capacity()
        {
            return _Capacity;
        }

        inline bool IsEmpty() { return _Length == 0; }

        inline bool IsFull() { return _Length == _Capacity; }

        virtual void Add(const T &Item) {}

        virtual void Add(T &&Item) {}

        virtual void Fill(const T &Item) {}

        virtual void Fill(const T &Item, size_t Count) {}

        virtual void Free() {}

        virtual void Free(size_t Count) {}

        ~Sequence() { delete[] _Content; }
    };
}