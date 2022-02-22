#pragma once

#include <iostream>
#include <tuple>

namespace Core
{
    template <typename... TArgs>
    class Coroutine {};

    template <typename... TArgs>
    class Coroutine<void(TArgs...)>
    {
    public:
        static constexpr size_t DefaultStackSize = 1024 * 4 * 4; // 4 pages of memory

        struct State
        {
            void *Stack;
            void *Base;
            void *Instruction;

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
        Coroutine() = default;

        Coroutine(size_t stackSize = DefaultStackSize) : StackSize(stackSize), Stack(new char[StackSize]) {}

        Coroutine(const Coroutine &) = delete;

        ~Coroutine()
        {
            delete[] Stack;
        }

        // Peroperties

        inline bool IsFinished()
        {
            return Finished;
        }

        // Funtionalities

        void Start(TArgs... Args)
        {
            asm volatile("" ::
                             : "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15");

            if (!Parent.Save())
            {
                // Normal

                // Change stack

                asm volatile(
                    "movq %0, %%rsp\n\t"
                    :
                    : "r"((void *)((size_t)Stack + StackSize)));

                // Call Invoker

                this->operator()(Args...);

                throw "Invalid scope"; // <- If user called normal return an exeption is thrown
            }

            // Returned
        }

        void Continue()
        {
            asm volatile("" ::
                             : "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15");

            if (!Parent.Save())
            {
                // Normal;

                // Call Invoker

                My.Restore();
            }
        }

        void Yield()
        {
            asm volatile("" ::
                             : "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15");

            if (!My.Save())
            {
                Parent.Restore();
            }
        }

        void Terminate()
        {
            //
        }

        // virtual void operator()(TArgs&... Args) {}

        virtual void operator()(TArgs... Args) {}
    };

    template <typename TReturn, typename... TArgs>
    class Coroutine<TReturn(TArgs...)>
    {
    public:
        static constexpr size_t DefaultStackSize = 1024 * 4 * 4; // 4 pages of memory

        struct State
        {
            void *Stack;
            void *Base;
            void *Instruction;

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

        struct Promise
        {
            bool HasResult = false;
            TReturn Result;
        };

    protected:
        State My;
        State Parent;

        size_t StackSize = 0;
        char *Stack;

        bool Finished = false;
        bool Started = false;

        TReturn Return;

    public:
        Coroutine() = default;

        Coroutine(size_t stackSize = DefaultStackSize) : StackSize(stackSize), Stack(new char[StackSize]) {}

        Coroutine(const Coroutine &) = delete;

        ~Coroutine()
        {
            delete[] Stack;
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

        // Funtionalities

        TReturn Start(TArgs... Args)
        {
            asm volatile("" ::
                             : "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15");

            if (!Parent.Save())
            {
                // Normal

                // Change stack

                asm volatile(
                    "movq %0, %%rsp\n\t"
                    :
                    : "r"((void *)((size_t)Stack + StackSize)));

                // Call Invoker

                this->operator()(Args...);

                throw "Invalid scope"; // <- If user called normal return an exeption is thrown
            }
            else
            {
                // Returned

                return Return;
            }
        }

        TReturn Continue()
        {
            asm volatile("" ::
                             : "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15");

            if (!Parent.Save())
            {
                // Normal;

                // Call Invoker

                My.Restore();
            }
            else
            {
                return Return;
            }
        }

        void Yield(TReturn Value)
        {
            Return = Value;

            asm volatile("" ::
                             : "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15");

            if (!My.Save())
            {
                Parent.Restore();
            }
        }

        void Terminate()
        {
            //
        }

        // virtual void operator()(TArgs&... Args) {}

        virtual TReturn operator()(TArgs... Args) {}
    };
}