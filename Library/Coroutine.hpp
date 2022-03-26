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
            void *Stack;
            void *Base;
            void *Instruction;

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
        State My;
        State Parent;

        size_t StackSize = 0;
        char *Stack;

        bool Finished = false;
        bool Started = false;

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
        using ArgsContainer = std::tuple<TArgs...>;

        ArgsContainer *_Args;

    public:
        Coroutine() = default;

        Coroutine(size_t stackSize) : __Coroutine_Helper(stackSize) {}

        ~Coroutine()
        {
            delete _Args;
        }

        // Peroperties

        ArgsContainer &Arguments()
        {
            return *_Args;
        }

        template <size_t TNumber>
        auto &Argument()
        {
            return std::get<TNumber>(*_Args);
        }

        // Functionalities

        void Start(const TArgs &...Args)
        {
            SaveRegs;

            if(_Args == nullptr)
            {
                _Args = new ArgsContainer(Args...);
            }
            else
            {
                *_Args = ArgsContainer(Args...);
            }

            if (!this->Parent.Save())
            {
                // Normal

                Started = true;

                // Change stack

                asm volatile(
                    "movq %0, %%rsp\t\n"
                    :
                    : "r"((void *)&Stack[StackSize]));

                // Call Invoker

                this->operator()();

                throw "Invalid continue"; // <- If user called normal return an exeption is thrown
            }

            // Returned
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
        using ArgsContainer = std::tuple<TArgs...>;

        // ArgsContainer *_Args;
        ArgsContainer *_Args = nullptr;
        TReturn Return;

    public:
        Coroutine() = default;

        Coroutine(size_t stackSize) : __Coroutine_Helper(stackSize) {}

        ~Coroutine()
        {
            delete _Args;
        }

        // Peroperties

        ArgsContainer &Arguments()
        {
            return *_Args;
        }

        template <size_t TNumber>
        auto &Argument()
        {
            return std::get<TNumber>(*_Args);
        }

        // Functionalities

        // @todo Take arguments by value

        TReturn Start(const TArgs &...Args)
        {
            SaveRegs;

            if(_Args == nullptr)
            {
                _Args = new ArgsContainer(Args...);
            }
            else
            {
                *_Args = ArgsContainer(Args...);
            }

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

                throw "Invalid scope"; // <- If user called normal return an exeption is thrown
            }
            else
            {
                // Returned

                return this->Return;
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
                return this->Return;
            }
        }

        void Yield(TReturn Value)
        {
            this->Return = Value;

            SaveRegs;

            if (!this->My.Save())
            {
                this->Parent.Restore();
            }
        }

        void Terminate(TReturn Value)
        {
            this->Return = Value;

            this->Finished = true;

            this->Parent.Restore();
        }

        virtual void operator()() = 0;
    };
}