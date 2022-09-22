#pragma once

#include <type_traits>
#include <stdexcept>
#include <utility>

namespace Core
{
    template <typename>
    class Function;

    template <typename TRet, typename... TArgs>
    class Function<TRet(TArgs...)>
    {
    public:
        using TObject = void *;
        using TInvoker = TRet (*)(void *, TArgs &&...);
        using TDestructor = void (*)(void const *);
        using TCopyConstructor = void (*)(void **, void const *);

        constexpr static size_t SmallSize = sizeof(TObject);

        constexpr Function() = default;

        constexpr Function(Function &&Other) noexcept : Object(Other.Object), Invoker(Other.Invoker), Destructor(Other.Destructor), CopyConstructor(Other.CopyConstructor), Hash(Other.Hash)
        {
            Other.Object = nullptr;
            Other.Invoker = nullptr;
            Other.Destructor = nullptr;
            Other.CopyConstructor = nullptr;
            Other.Hash = nullptr;
        }

        constexpr Function(Function const &Other) : Invoker(Other.Invoker), Destructor(Other.Destructor), CopyConstructor(Other.CopyConstructor), Hash(Other.Hash)
        {
            Other.AssertCopyable();

            CopyConstructor(&Object, Other.Object);
        }

        constexpr Function(std::nullptr_t) noexcept {};

        template <typename T, typename... TCArgs>
        constexpr Function(std::type_identity<T>, TCArgs &&...CArgs) : Hash(typeid(T).name())
        {
            if constexpr (sizeof(T) > SmallSize)
            {
                Object = new T(std::forward<TCArgs>(CArgs)...);

                Invoker = [](void *Item, TArgs &&...Args)
                {
                    return static_cast<T *>(Item)->operator()(std::forward<TArgs>(Args)...);
                };

                if constexpr (!std::is_trivially_destructible_v<T>)
                {
                    Destructor = [](void const *Item)
                    {
                        delete static_cast<T const *>(Item);
                    };
                }
                else
                {
                    Destructor = nullptr;
                }

                if constexpr (std::is_copy_constructible_v<T> || std::is_trivially_constructible_v<T>)
                {
                    CopyConstructor = [](void **Self, void const *Other)
                    {
                        *Self = new T(*static_cast<T const *>(Other));
                    };
                }
                else
                {
                    CopyConstructor = nullptr;
                }
            }
            else
            {
                std::construct_at(static_cast<T *>(static_cast<void *>(&Object)), std::forward<TCArgs>(CArgs)...);

                Invoker = [](void *Item, TArgs &&...Args)
                {
                    return static_cast<T *>(static_cast<void *>(&Item))->operator()(std::forward<TArgs>(Args)...);
                };

                if constexpr (!std::is_trivially_destructible_v<T>)
                {
                    Destructor = [](void const *Item)
                    {
                        static_cast<T const *>(static_cast<void *>(&Item))->~T();
                    };
                }
                else
                {
                    Destructor = nullptr;
                }

                if constexpr (std::is_copy_constructible_v<T> || std::is_trivially_constructible_v<T>)
                {
                    CopyConstructor = [](void **Self, void const *Other)
                    {
                        std::construct_at(static_cast<T *>(static_cast<void *>(Self)), *static_cast<T const *>(static_cast<void *>(&Other)));
                    };
                }
                else
                {
                    CopyConstructor = nullptr;
                }
            }
        }

        template <typename TFunctor>
        constexpr Function(TFunctor &&Functor) noexcept
            : Function(std::type_identity<TFunctor>{}, std::forward<TFunctor>(Functor)) {}

        constexpr Function(TRet (*Function)(TArgs...)) noexcept
            : Object(reinterpret_cast<void *>(Function)),
              Invoker(
                  [](void *Item, TArgs &&...Args)
                  {
                      return (reinterpret_cast<TRet (*)(TArgs...)>(Item))(std::forward<TArgs>(Args)...);
                  }),
              Destructor(nullptr),
              CopyConstructor(
                  [](void **Self, void const *Other)
                  {
                      // @todo Fix this!!

                      *Self = (void *)Other;
                  }),
              Hash(typeid(decltype(Function)).name()) {}

        template <typename TFunctor>
        constexpr Function &operator=(TFunctor &&Functor)
        requires(!std::is_same_v<std::decay_t<TFunctor>, Function>)
        {
            // @todo Optimize this

            *this = Function(std::type_identity<TFunctor>{}, std::forward<TFunctor>(Functor));

            return *this;
        }

        constexpr Function &operator=(Function &&Other) noexcept
        {
            if (this != &Other)
            {
                Clear();

                Object = Other.Object;
                Invoker = Other.Invoker;
                CopyConstructor = Other.CopyConstructor;
                Destructor = Other.Destructor;
                Hash = Other.Hash;

                Other.Object = nullptr;
                Other.Invoker = nullptr;
                Other.CopyConstructor = nullptr;
                Other.Destructor = nullptr;
                Other.Hash = nullptr;
            }

            return *this;
        }

        constexpr Function &operator=(Function const &Other)
        {
            Other.AssertCopyable();

            Clear();

            Invoker = Other.Invoker;
            CopyConstructor = Other.CopyConstructor;
            Destructor = Other.Destructor;
            Hash = Other.Hash;

            CopyConstructor(&Object, Other.Object);
            return *this;
        }

        constexpr ~Function()
        {
            Clear();
        }

        constexpr inline bool IsCopyable() const
        {
            return CopyConstructor != nullptr;
        }

        constexpr void Clear()
        {
            if (Destructor)
            {
                Destructor(Object);
                Destructor = nullptr;
            }

            Invoker = nullptr;
        }

        template <typename T>
        constexpr T *Target()
        {
            if (typeid(T).name() != Hash)
                throw std::bad_cast();

            return static_cast<T *>(Object);
        }

        constexpr inline TRet operator()(TArgs... Args) const
        {
            return Invoker(Object, std::forward<TArgs>(Args)...);
        }

        constexpr inline operator bool() const
        {
            return bool(Invoker);
        }

    protected:
        TObject Object = nullptr;
        TInvoker Invoker = nullptr;
        TDestructor Destructor = nullptr;
        TCopyConstructor CopyConstructor = nullptr;
        char const *Hash = nullptr;

        constexpr inline void AssertCopyable() const
        {
            if (!IsCopyable())
                throw std::runtime_error("No suitable copy constructor");
        }
    };
}