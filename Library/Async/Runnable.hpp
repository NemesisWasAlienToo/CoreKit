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