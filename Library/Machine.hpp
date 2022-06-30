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

// @todo Maybe check the coroutine is already terminated
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
        mutable size_t _State = 0;
        mutable bool _IsFinished = false;

    public:
        bool IsStarted() const { return _State; }
        bool IsFinished() const { return _IsFinished; }
        void Terminate() const { _IsFinished = true; }

        void Reset() const
        {
            _State = 0;
            _IsFinished = false;
        }

        virtual TReturn operator()(TArgs...) = 0;
    };
}
