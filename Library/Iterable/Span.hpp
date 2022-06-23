#pragma once

#include <optional>
#include <initializer_list>

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

        public:
            Span() = default;
            Span(size_t Size) : _Length(Size), _Content(new T[Size]) {}

            Span(size_t Size, const T &Value) : _Length(Size), _Content(new T[Size])
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    _Content[i] = Value;
                }
            }

            Span(Span &&Other) : _Length(Other._Length), _Content(Other._Content)
            {
                Other._Content = nullptr;
                Other._Length = 0;
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

            Span(std::initializer_list<T> list) : _Length(list.size()), _Content(new T[list.size()])
            {
                size_t i = 0;
                for (auto &item : list)
                {
                    _Content[i] = item;
                    i++;
                }
            }

            ~Span()
            {
                delete[] _Content;
                _Content = nullptr;
            }

            Span &operator=(Span const &Other)
            {
                if (this != &Other)
                {
                    _Length = Other._Length;

                    delete[] _Content;
                    _Content = new T[_Length];

                    for (size_t i = 0; i < _Length; i++)
                    {
                        _Content[i] = Other._Content[i];
                    }
                }

                return *this;
            }

            Span &operator=(Span &&Other)
            {
                if (this != &Other)
                {
                    delete[] _Content;

                    _Content = Other._Content;
                    _Length = Other._Length;

                    Other._Content = nullptr;
                    Other._Length = 0;
                }

                return *this;
            }

            inline T *Content()
            {
                return _Content;
            }

            inline T const *Content() const
            {
                return _Content;
            }

            inline size_t Length() const
            {
                return _Length;
            }

            T &First()
            {
                return this->operator[](0);
            }

            T &Last()
            {
                return this->operator[](_Length - 1);
            }

            T const &First() const
            {
                return this->operator[](0);
            }

            T const &Last() const
            {
                return this->operator[](_Length - 1);
            }

            void Resize(size_t const NewSize)
            {
                auto NewData = new T[NewSize];
                auto Bound = std::min(NewSize, _Length);

                for (size_t i = 0; i < Bound; i++)
                {
                    NewData[i] = _Content[i];
                }

                delete[] _Content;

                _Length = NewSize;

                _Content = NewData;
            }

            template <typename TCallback>
            std::optional<size_t> Contains(TCallback Callback) const
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    if (Callback(_Content[i]))
                        return i;
                }

                return std::nullopt;
            }

            template <typename TCallback>
            void ForEach(TCallback Action)
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    Action(_Content[i]);
                }
            }

            template <typename TCallback>
            void ForEach(TCallback Action) const
            {
                for (size_t i = 0; i < _Length; i++)
                {
                    Action(_Content[i]);
                }
            }

            T &operator[](size_t const Index)
            {
                if (Index >= _Length)
                    throw std::out_of_range("");

                return _Content[Index];
            }

            T const &operator[](size_t const Index) const
            {
                if (Index >= _Length)
                    throw std::out_of_range("");

                return _Content[Index];
            }
        };
    }
}