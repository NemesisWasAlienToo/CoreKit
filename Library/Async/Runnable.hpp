#pragma once

namespace Core::Async
{
    class Runnable
    {
    protected:
        volatile bool Running{false};

        inline void Run()
        {
            Running = true;
        }

        inline void Stop()
        {
            Running = false;
        }

        inline bool IsRunning()
        {
            return Running;
        }
    };
}

// #include <atomic>

// class Runnable
// {
// protected:
//     std::atomic<bool> Running{false};

//     inline void Run()
//     {
//         Running.store(true);
//     }

//     inline void Stop()
//     {
//         Running.store(false);
//     }

//     inline bool IsRunning()
//     {
//         return Running.load(std::memory_order_relaxed);
//     }
// };