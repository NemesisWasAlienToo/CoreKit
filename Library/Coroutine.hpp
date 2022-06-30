#pragma once

#include <iostream>
#include <tuple>
#include <type_traits>

#define SaveRegs asm volatile("" :: \
                                  : "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15")

#define StackPointer(RES) asm("movq %%rsp, %0" \
                              : "=r"(RES))

namespace Core
{
    class __Coroutine_Helper
    {
    public:
        struct State
        {
            void *Stack = nullptr;
            void *Base = nullptr;
            void *Instruction = nullptr;

            State() = default;

            State(State &&Other) : Stack(Other.Stack), Base(Other.Base), Instruction(Other.Instruction)
            {
                Other.Stack = nullptr;
                Other.Base = nullptr;
                Other.Instruction = nullptr;
            }

            State(const State &) = delete;

            State &operator=(State &&Other)
            {
                Stack = Other.Stack;
                Base = Other.Base;
                Instruction = Other.Instruction;

                Other.Stack = nullptr;
                Other.Base = nullptr;
                Other.Instruction = nullptr;

                return *this;
            }

            State &operator=(const State &) = delete;

            bool __attribute__((noinline)) Save()
            {
                bool Result;

                asm volatile(
                    "movq %%rsp, %0\n\t"
                    "movq %%rbp, %1\n\t"
                    "movq $1f, %2\n\t"
                    "movb $0, %3\n\t"
                    "jmp 2f\n\t"
                    "1:"
                    "movb $1, %3\n\t"
                    "2:"
                    : "=m"(Stack), "=m"(Base), "=m"(Instruction), "=r"(Result)
                    :
                    : "memory");

                return Result;
            }

            [[noreturn]] inline __attribute__((always_inline)) void Restore()
            {
                asm volatile(
                    "movq %0, %%rsp\n\t"
                    "movq %1, %%rbp\n\t"
                    "jmp *%2"
                    :
                    : "m"(Stack), "r"(Base), "r"(Instruction));
            }
        };

    protected:
        mutable State My;
        mutable State Parent;

        mutable size_t StackSize = 0;
        mutable char *Stack;

        mutable bool Finished = false;
        mutable bool Started = false;

    public:
        __Coroutine_Helper() = default;

        __Coroutine_Helper(size_t stackSize) : StackSize(stackSize), Stack(new char[StackSize]) {}

        __Coroutine_Helper(const __Coroutine_Helper &) = delete;

        __Coroutine_Helper(__Coroutine_Helper &&Other) : My(std::move(Other.My)), Parent(std::move(Other.Parent)),
                                                         StackSize(Other.StackSize), Stack(Other.Stack), Finished(Other.Finished), Started(Other.Started)
        {
            Other.StackSize = 0;
            Other.Stack = nullptr;

            Other.Finished = false;
            Other.Started = false;
        }

        virtual ~__Coroutine_Helper()
        {
            delete[] Stack;
        }

        __Coroutine_Helper &operator=(const __Coroutine_Helper &) = delete;

        __Coroutine_Helper &operator=(__Coroutine_Helper &&Other)
        {
            My = std::move(Other.My);
            Parent = std::move(Other.Parent);

            StackSize = Other.StackSize;
            Stack = Other.Stack;

            Finished = Other.Finished;
            Started = Other.Started;

            Other.StackSize = 0;
            Other.Stack = nullptr;

            Other.Finished = false;
            Other.Started = false;

            return *this;
        }

        // Peroperties

        inline bool IsFinished()
        {
            return Finished;
        }

        inline bool IsStarted()
        {
            return Started;
        }
    };

    template <typename... TArgs>
    class Coroutine
    {
    };

    template <typename... TArgs>
    class Coroutine<void(TArgs...)> : public __Coroutine_Helper
    {
    protected:
        static_assert(!(std::is_reference_v<TArgs> || ...), "Coroutine arguments can't be reference, use pointers instead");

        template <size_t N>
        using TArgument = std::tuple_element_t<N, std::tuple<TArgs...>>;

        using ArgsContainer = std::tuple<std::remove_const_t<TArgs>...>;

        ArgsContainer _Args;

    public:
        Coroutine() = default;

        Coroutine(size_t stackSize) : __Coroutine_Helper(stackSize) {}

        ~Coroutine() = default;

        // Peroperties

        template <size_t TNumber>
        constexpr TArgument<TNumber> &
        Argument()
        {
            return std::get<TNumber>(_Args);
        }

        // Functionalities

        void Start(std::remove_reference_t<TArgs> &...Args)
        {
            SaveRegs;

            _Args = ArgsContainer(std::forward<TArgs>(Args)...);

            if (!this->Parent.Save())
            {
                // Normal

                Started = true;
                Finished = false;

                // Change stack

                asm volatile(
                    "movq %0, %%rsp\t\n"
                    :
                    : "r"((void *)&Stack[StackSize]));

                // Call Invoker

                this->operator()();

                throw "Invalid use of return in coroutine"; // <- If user called normal return an exeption is thrown
            }
        }

        void Start(std::remove_reference_t<TArgs> &&...Args)
        {
            SaveRegs;

            _Args = ArgsContainer(std::forward<TArgs>(Args)...);

            if (!this->Parent.Save())
            {
                // Normal

                Started = true;
                Finished = false;

                // Change stack

                asm volatile(
                    "movq %0, %%rsp\t\n"
                    :
                    : "r"((void *)&Stack[StackSize]));

                // Call Invoker

                this->operator()();

                throw "Invalid use of return in coroutine"; // <- If user called normal return an exeption is thrown
            }
        }

        void Continue()
        {
            SaveRegs;

            if (!Parent.Save())
            {
                // Normal;

                // Call Invoker

                My.Restore();
            }
        }

        void Yield()
        {
            SaveRegs;

            if (!My.Save())
            {
                Parent.Restore();
            }
        }

        void Terminate()
        {
            Finished = true;

            Parent.Restore();
        }

        virtual void operator()() = 0;
    };

    template <typename TReturn, typename... TArgs>
    class Coroutine<TReturn(TArgs...)> : public __Coroutine_Helper
    {
    protected:
        static_assert(!(std::is_reference_v<TArgs> || ...), "Coroutine arguments can't be reference, use pointers instead");
        static_assert(!std::is_reference_v<TReturn>, "Coroutine return type must not be reference, use pointers instead");

        template <size_t N>
        using TArgument = std::tuple_element_t<N, std::tuple<TArgs...>>;

        // Types

        using ArgsContainer = std::tuple<std::remove_const_t<TArgs>...>;

        // Internal variables

        ArgsContainer _Args;
        TReturn _Return;

    public:
        Coroutine() = default;

        Coroutine(size_t stackSize) : __Coroutine_Helper(stackSize) {}

        ~Coroutine() = default;

        // Peroperties

        template <size_t TNumber>
        constexpr TArgument<TNumber> &
        Argument()
        {
            return std::get<TNumber>(_Args);
        }

        // Functionalities

        TReturn Start(std::remove_reference_t<TArgs> &...Args)
        {
            SaveRegs;

            _Args = ArgsContainer(std::forward<TArgs>(Args)...);

            if (!this->Parent.Save())
            {
                // Normal

                Started = true;
                Finished = false;

                // Change stack

                asm volatile(
                    "movq %0, %%rsp\t\n"
                    :
                    : "r"((void *)&Stack[StackSize]));

                // Call Invoker

                this->operator()();

                throw "Invalid use of return in coroutine"; // <- If user called normal return an exeption is thrown
            }
            else
            {
                // Returned

                return std::forward<TReturn>(_Return);
            }
        }

        TReturn Start(std::remove_reference_t<TArgs> &&...Args)
        {
            SaveRegs;

            _Args = ArgsContainer(std::forward<TArgs>(Args)...);

            if (!this->Parent.Save())
            {
                // Normal

                Started = true;
                Finished = false;

                // Change stack

                asm volatile(
                    "movq %0, %%rsp\t\n"
                    :
                    : "r"((void *)&Stack[StackSize]));

                // Call Invoker

                this->operator()();

                throw "Invalid use of return in coroutine"; // <- If user called normal return an exeption is thrown
            }
            else
            {
                // Returned

                return std::forward<TReturn>(_Return);
            }
        }

        TReturn Continue()
        {
            SaveRegs;

            if (!this->Parent.Save())
            {
                // Normal;

                // Call Invoker

                this->My.Restore();
            }
            else
            {
                return std::forward<TReturn>(_Return);
            }
        }

        void Yield(std::remove_reference_t<TReturn> &Value)
        {
            SaveRegs;

            _Return = std::forward<TReturn>(Value);

            if (!this->My.Save())
            {
                this->Parent.Restore();
            }
        }

        void Yield(std::remove_reference_t<TReturn> &&Value)
        {
            SaveRegs;

            _Return = std::forward<TReturn>(Value);

            if (!this->My.Save())
            {
                this->Parent.Restore();
            }
        }

        void Terminate(std::remove_reference_t<TReturn> &Value)
        {
            _Return = std::forward<TReturn>(Value);

            this->Finished = true;

            this->Parent.Restore();
        }

        void Terminate(std::remove_reference_t<TReturn> &&Value)
        {
            _Return = std::forward<TReturn>(Value);

            this->Finished = true;

            this->Parent.Restore();
        }

        virtual void operator()() = 0;
    };
}