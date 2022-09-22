#pragma once

#include <utility>
#include <typeinfo>
#include <type_traits>

namespace Core
{
    template <typename T, typename E>
    class Result
    {
    public:
        template <typename TArg>
        requires(!std::is_same_v<std::decay_t<TArg>, Result>)
        Result(TArg &&Arg)
        {
            if constexpr (std::is_constructible_v<T, TArg>)
            {
                HasValue = true;
                new (&Storage) T(std::forward<TArg>(Arg));
            }
            else
            {
                HasValue = false;
                new (&Storage) E(std::forward<TArg>(Arg));
            }
        }

        Result(Result const &Other) : HasValue(Other.HasValue)
        {
            if (Other.HasValue)
            {
                new (&Storage) T(*reinterpret_cast<T const *>(&Other.Storage));
            }
            else
            {
                new (&Storage) E(*reinterpret_cast<E const *>(&Other.Storage));
            }
        }

        Result(Result &&Other) = delete;

        ~Result()
        {
            Clear();
        }

        template <typename TArg>
        requires(!std::is_same_v<std::decay_t<TArg>, Result>)
        Result &operator=(TArg &&Arg)
        {
            Clear();

            if constexpr (std::is_constructible_v<T, TArg>)
            {
                HasValue = true;
                new (&Storage) T(std::forward<TArg>(Arg));
            }
            else
            {
                HasValue = false;
                new (&Storage) E(std::forward<TArg>(Arg));
            }

            return *this;
        }

        Result &operator=(Result const &Other)
        {
            Clear();
            HasValue = Other.HasValue;

            if (Other.HasValue)
            {
                new (&Storage) T(*reinterpret_cast<T const *>(&Other.Storage));
            }
            else
            {
                new (&Storage) E(*reinterpret_cast<E const *>(&Other.Storage));
            }

            return *this;
        }

        Result &operator=(Result &&Other) = delete;

        inline operator bool()
        {
            return HasValue;
        }

        inline void Unwrap()
        {
            throw *reinterpret_cast<E *>(&Storage);
        }

        inline T &Value()
        {
            if (!HasValue)
                throw std::runtime_error("Instance contains no value");

            return *reinterpret_cast<T *>(&Storage);
        }

        inline E &Error()
        {
            if (HasValue)
                throw std::runtime_error("Instance contains no error");

            return *reinterpret_cast<E *>(&Storage);
        }

    protected:
        static constexpr size_t data_size = std::max(sizeof(T), sizeof(E));
        static constexpr size_t data_align = std::max(sizeof(T), sizeof(E));
        using TStorage = typename std::aligned_storage<data_size, data_align>::type;

        bool HasValue;
        TStorage Storage;

        void Clear()
        {
            if (HasValue)
            {
                if constexpr (!std::is_trivially_destructible_v<T>)
                    reinterpret_cast<T *>(&Storage)->~T();
            }
            else
            {
                if constexpr (!std::is_trivially_destructible_v<E>)
                    reinterpret_cast<E *>(&Storage)->~E();
            }
        }
    };
}