#include <iostream>
#include <type_traits>

namespace Core
{
#include <iostream>
#include <type_traits>
#include <functional>

    template <typename>
    class Function;

    template <typename TRet, typename... TArgs>
    class Function<TRet(TArgs...)>
    {
    public:
        using TInvoker = TRet (*)(void *, TArgs &&...);
        using TDestructor = void (*)(void const *);
        using TCopyConstructor = void (*)(void **, void const *);

        Function() = default;

        Function(Function &&Other) noexcept : Object(Other.Object), Invoker(Other.Invoker), Destructor(Other.Destructor), CopyConstructor(Other.CopyConstructor), Hash(Other.Hash)
        {
            Other.Object = nullptr;
            Other.Invoker = nullptr;
            Other.Destructor = nullptr;
            Other.CopyConstructor = nullptr;
            Other.Hash = 0;
        }

        Function(Function const &Other) : Invoker(Other.Invoker), Destructor(Other.Destructor), CopyConstructor(Other.CopyConstructor), Hash(Other.Hash)
        {
            AssertCopyable();

            CopyConstructor(&Object, Other.Object);
        }

        Function(std::nullptr_t) {};

        template <typename TFunctor>
        Function(TFunctor &&Functor) noexcept : Object(new std::decay_t<TFunctor>(std::forward<TFunctor>(Functor))),
                                                Invoker(
                                                    [](void *Item, TArgs... Args)
                                                    {
                                                        using T = std::decay_t<TFunctor>;
                                                        return static_cast<T *>(Item)->operator()(std::forward<TArgs>(Args)...);
                                                    }),
                                                Destructor(
                                                    [](void const *Item)
                                                    {
                                                        using T = std::decay_t<TFunctor>;
                                                        static_cast<T const *>(Item)->~T();
                                                    }),

                                                Hash(typeid(TFunctor).hash_code())
        {
            if constexpr (std::is_copy_constructible_v<std::decay_t<TFunctor>> || std::is_trivially_constructible_v<std::decay_t<TFunctor>>)
            {
                CopyConstructor = [](void **Self, void const *Other)
                {
                    *Self = new std::decay_t<TFunctor>(*static_cast<std::decay_t<TFunctor> const *>(Other));
                };
            }
            else
            {
                CopyConstructor = nullptr;
            }
        }

        Function &operator=(Function &&Other)
        {
            if (this != &Other)
            {
                Clear();

                Invoker = Other.Invoker;
                CopyConstructor = Other.CopyConstructor;
                Destructor = Other.Destructor;
                Hash = Other.Hash;

                Other.Invoker = nullptr;
                Other.CopyConstructor = nullptr;
                Other.Destructor = nullptr;
                Other.Hash = 0;
            }

            return *this;
        }

        Function &operator=(Function const &Other)
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

        ~Function()
        {
            Clear();
        }

        template <typename TFunctor>
        static Function From(TFunctor &&Functor)
        {
            Function Result;

            using T = std::decay_t<TFunctor>;

            Result.Object = new T(std::forward<TFunctor>(Functor));

            Result.Invoker = [](void *Item, TArgs... Args)
            {
                return static_cast<T *>(Item)->operator()(std::forward<TArgs>(Args)...);
            };

            Result.Destructor = [](void const *Item)
            {
                static_cast<T const *>(Item)->~T();
                delete static_cast<T const *>(Item);
            };

            Result.Hash = typeid(T).hash_code();

            if constexpr (std::is_copy_constructible_v<T> || std::is_trivially_constructible_v<T>)
            {
                Result.CopyConstructor = [](void **Self, void const *Other)
                {
                    *Self = new T(*static_cast<T const *>(Other));
                };
            }
            else
            {
                Result.CopyConstructor = nullptr;
            }

            return Result;
        }

        inline bool IsCopyable() const
        {
            return CopyConstructor != nullptr;
        }

        void Clear()
        {
            if (Destructor && Object)
                Destructor(Object);
        }

        template <typename T>
        T *Target()
        {
            if (typeid(T).hash_code() != Hash)
                throw std::bad_cast();

            return static_cast<T *>(Object);
        }

        template <typename... RTArgs>
        TRet operator()(RTArgs &&...Args)
        {
            return Invoker(Object, std::forward<RTArgs>(Args)...);
        }

        operator bool()
        {
            return (Invoker && Object);
        }

    protected:
        void *Object = nullptr;
        TInvoker Invoker = nullptr;
        TDestructor Destructor = nullptr;
        TCopyConstructor CopyConstructor = nullptr;
        size_t Hash = 0;

        inline void AssertCopyable() const
        {
            if (!IsCopyable())
                throw std::runtime_error("No suitable copy constructor");
        }
    };
}