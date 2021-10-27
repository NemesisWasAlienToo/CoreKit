#pragma once

#include "Iterable/Iterable.cpp"

namespace Core
{
    namespace Iterable
    {
        template <typename T>
        class Span
        {
        private:
            T *_Content = nullptr;
            size_t _Length = 0;

            _FORCE_INLINE inline T &_ElementAt(size_t Index)
            {
                return this->_Content[Index];
            }

            _FORCE_INLINE inline const T &_ElementAt(size_t Index) const
            {
                return this->_Content[Index];
            }

        public:
            T *Content() const
            {
                return _Content;
            }

            size_t Length()
            {
                return _Length;
            }

            Span(T *Array, size_t Size) : _Content(Array), _Length(Size) {}
            Span(Span &&Other) : _Content(Other._Content), _Length(Other._Length) {}

            ~Span()
            {
                delete[] _Content;
                _Content = nullptr;
            }

            bool Contains(const T &Item) const
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    if (_ElementAt(i) == Item)
                        return true;
                }

                return false;
            }

            bool Contains(const T &Item, int &Index) const
            {
                for (size_t i = 0; i < _Length; i++)
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

            T &operator[](const size_t &Index)
            {
                if (Index >= _Length)
                    throw std::out_of_range("");

                return _ElementAt(Index);
            }

            const T &operator[](const size_t &Index) const
            {
                if (Index >= _Length)
                    throw std::out_of_range("");

                return _ElementAt(Index);
            }

            Span &operator=(const Span &Other) = delete;

            bool operator==(const Span &Other) noexcept
            {
                return this->_Content == Other->_Content;
            }

            bool operator!=(const Span &Other) noexcept
            {
                return this->_Content != Other->_Content;
            }
        };
    }
}