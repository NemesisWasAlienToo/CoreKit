#pragma once

#include <iostream>
#include <tuple>

namespace Core
{

#define CO_YIELD(RES)  \
    _State = __LINE__; \
    return RES;        \
    case __LINE__:

#define CO_TERMINATE(RES) \
    _State = 0;           \
    _IsFinished = true;   \
    return RES;

#define CO_START    \
    switch (_State) \
    {               \
    case 0:

#define CO_END \
    }          \
    throw std::invalid_argument("");

    template <typename... T>
    class Machine
    {
    };

    template <typename TReturn, typename... TArgs>
    class Machine<TReturn(TArgs...)>
    {
    protected:
        static_assert(!(std::is_reference_v<TArgs> || ...), "Coroutine arguments can't be reference, use pointers instead");
        static_assert(!std::is_reference_v<TReturn>, "Coroutine return type must not be reference, use pointers instead");

        template <size_t N>
        using TArgument = std::tuple_element_t<N, std::tuple<TArgs...>>;

        using ArgsContainer = std::tuple<std::remove_const_t<TArgs>...>;

        size_t _State = 0;
        bool _IsFinished = false;
        ArgsContainer _Args;

    public:
        bool IsStarted() { return _State; }
        bool IsFinished() const { return _IsFinished == true; }
        void Terminate() { _IsFinished = true; }

        template <size_t TNumber>
        constexpr TArgument<TNumber> &
        Argument()
        {
            return std::get<TNumber>(_Args);
        }

        inline TReturn Start(TArgs... Args)
        {
            _State = 0;
            _Args = ArgsContainer(std::forward<TArgs&&>(Args)...);
            _IsFinished = false;

            if constexpr (std::is_same_v<void, TReturn>)
            {
                this->operator()();
            }
            else
            {
                return this->operator()();
            }
        }

        inline TReturn Continue()
        {
            if constexpr (std::is_same_v<void, TReturn>)
            {
                this->operator()();
            }
            else
            {
                return this->operator()();
            }
        }

        virtual TReturn operator()() = 0;
    };
}
