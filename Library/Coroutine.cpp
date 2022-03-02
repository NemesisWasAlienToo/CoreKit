#pragma once

#include <strings.h>

namespace Core
{
    namespace
    {
        class __Coroutine_Helper
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
            __Coroutine_Helper() = default;

            __Coroutine_Helper(size_t stackSize) : StackSize(stackSize), Stack(new char[StackSize]) {}

            __Coroutine_Helper(const __Coroutine_Helper &) = delete;

            ~__Coroutine_Helper()
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
        };
    }

    template <typename... TArgs>
    class Coroutine {};

    template <typename... TArgs>
    class Coroutine<void(TArgs...)> : public __Coroutine_Helper
    {
    public:
        Coroutine() = default;

        Coroutine(size_t stackSize = __Coroutine_Helper::DefaultStackSize) : __Coroutine_Helper(stackSize) {}

        // Funtionalities

        void Start(TArgs... Args)
        {
            asm volatile("" ::
                             : "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15");

            if (!this->Parent.Save())
            {
                // Normal

                // Change stack

                asm volatile(
                    "movq %0, %%rsp\n\t"
                    :
                    : "r"((void *)((size_t)Stack + StackSize)));

                // Call Invoker

                this->operator()(std::forward<TArgs...>(Args...));

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
            Finished = true;

            Parent.Restore();
        }

        virtual void operator()(TArgs... Args) = 0;
    };

    template <typename TReturn, typename... TArgs>
    class Coroutine<TReturn(TArgs...)> : public __Coroutine_Helper
    {
    protected:
        TReturn Return;

    public:
        Coroutine() = default;

        Coroutine(size_t stackSize = __Coroutine_Helper::DefaultStackSize) : __Coroutine_Helper(stackSize) {}

        // Funtionalities

        TReturn Start(TArgs... Args)
        {
            asm volatile("" ::
                             : "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15");

            if (!this->Parent.Save())
            {
                // Normal

                // Change stack

                asm volatile(
                    "movq %0, %%rsp\n\t"
                    :
                    : "r"((void *)((size_t)this->Stack + this->StackSize)));

                // Call Invoker

                this->operator()(std::forward<TArgs...>(Args...));

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
            asm volatile("" ::
                             : "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15");

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

            asm volatile("" ::
                             : "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15");

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

        virtual void operator()(TArgs... Args) = 0;
    };
}