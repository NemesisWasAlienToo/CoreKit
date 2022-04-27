#pragma once

#include "Iterable/Iterable.hpp"

namespace Core
{
    namespace Iterable
    {
        template <typename T>
        class Span
        {
        private:
            size_t _Length = 0;
            T *_Content = nullptr;

            _FORCE_INLINE inline T &_ElementAt(size_t Index)
            {
                return this->_Content[Index];
            }

            _FORCE_INLINE inline const T &_ElementAt(size_t Index) const
            {
                return this->_Content[Index];
            }

        public:
            Span() = default;
            Span(size_t Size) : _Length(Size), _Content(new T[Size]) {}
            Span(Span &&Other) : _Length(Other._Length), _Content(Other._Content)
            {
                Other._Content = nullptr;
                Other._Length = 0;
            }

            Span(size_t Size, const T &Value) : _Length(Size), _Content(new T[Size])
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    _Content[i] = Value;
                }
            }

            Span(const Span &Other) : _Length(Other._Length), _Content(new T[Other._Length])
            {
                for (size_t i = 0; i < Other._Length; i++)
                {
                    _Content[i] = Other._Content[i];
                }
            }

            Span(const T *Array, size_t Size) : _Length(Size), _Content(new T[Size])
            {
                for (size_t i = 0; i < Size; i++)
                {
                    _Content[i] = Array[i];
                }
            }

            ~Span()
            {
                delete[] _Content;
                _Content = nullptr;
            }

            inline T *Content()
            {
                return _Content;
            }

            inline const T *Content() const
            {
                return _Content;
            }

            inline size_t Length() const
            {
                return _Length;
            }

            inline void Length(size_t Length)
            {
                _Length = Length;
            }

            void Resize(size_t NewSize)
            {
                auto NewData = new T[NewSize];
                auto Bound = std::min(NewSize, _Length);

                for (size_t i = 0; i < Bound; i++)
                {
                    NewData[i] = _Content[i];
                }

                free(_Content);

                _Length = NewSize;

                _Content = NewData;
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

            template <typename TCallback>
            void ForEach(TCallback Action)
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    Action(_ElementAt(i));
                }
            }

            template <typename TCallback>
            void ForEach(TCallback Action) const
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    Action(_ElementAt(i));
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

            Span &operator=(const Span &Other)
            {
                if (this == &Other)
                {
                    return *this;
                }

                _Length = Other._Length;

                delete[] _Content;
                _Content = new T[_Length];

                for (size_t i = 0; i < _Length; i++)
                {
                    _Content[i] = Other._Content[i];
                }

                return *this;
            }

            Span &operator=(Span &&Other)
            {
                if (this == &Other)
                {
                    return *this;
                }
                
                delete[] _Content;

                _Content = Other._Content;
                _Length = Other._Length;

                Other._Content = nullptr;
                Other._Length = 0;

                return *this;
            }

            // Operators

            bool operator==(const Span &Other) noexcept
            {
                if (_Content == Other._Content)
                    return true;

                for (size_t i = 0; i < _Length; i++)
                {
                    if (_Content[i] != Other._Content[i])
                        return false;
                }

                return true;
            }

            bool operator!=(const Span &Other) noexcept
            {
                if (_Content != Other._Content)
                    return true;

                for (size_t i = 0; i < _Length; i++)
                {
                    if (_Content[i] == Other._Content[i])
                        return false;
                }

                return true;
            }
        };
    }
}