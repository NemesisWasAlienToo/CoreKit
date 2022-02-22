#pragma once

#include <iostream>
#include <string>
#include <poll.h>
#include <sys/timerfd.h>
#include <system_error>

#include "Descriptor.cpp"
#include "Duration.cpp"

namespace Core
{
    class Timer : public Descriptor
    {
    public:
        enum TimerTypes
        {
            RealTime = CLOCK_REALTIME,
            RealTimeAlarm = CLOCK_REALTIME_ALARM,
            BootTime = CLOCK_BOOTTIME,
            BootTimeAlarm = CLOCK_BOOTTIME_ALARM,
            Monotonic = CLOCK_MONOTONIC,
        };

        enum TimerFlags
        {
            CloseOnExec = TFD_CLOEXEC,
            NonBlocking = TFD_NONBLOCK,
        };

        // ### Constructors

        Timer(int Handler) : Descriptor(Handler) {}
        
        Timer(TimerTypes Type, int Flags)
        {
            int Result = timerfd_create(Type, Flags);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            _INode = Result;
        }

        Timer(const Timer &Other) : Descriptor(Other) {}

        ~Timer() { Close(); }

        // ### Functionalities

        void Set(Duration Initial, Duration Periodic)
        {
            struct itimerspec _Duration = {{0, 0}, {0, 0}};

            Initial.Fill(_Duration.it_value);
            Periodic.Fill(_Duration.it_interval);

            if (timerfd_settime(_INode, 0, &_Duration, NULL) < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }
        }

        void Set(Duration duration)
        {
            struct itimerspec _Duration = {0, 0};

            duration.Fill(_Duration.it_value);

            if (timerfd_settime(_INode, 0, &_Duration, NULL) < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }
        }

        void Stop()
        {
            struct itimerspec _Duration = {0, 0};

            if (timerfd_settime(_INode, 0, &_Duration, NULL) < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }
        }

        Duration Period()
        {
            struct itimerspec _Duration = {0, 0};

            if (timerfd_gettime(_INode, &_Duration) < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return _Duration.it_interval;
        }

        Duration Left()
        {
            struct itimerspec _Duration = {0, 0};

            if (timerfd_gettime(_INode, &_Duration) < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return _Duration.it_value;
        }

        uint64_t Value() const
        {
            unsigned long Value;

            int Result = read(_INode, &Value, sizeof Value);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }

            return Value;
        }

        uint64_t Await(int TimeoutMS = -1) const
        {
            _POLLFD PollStruct = {.fd = _INode, .events = In};

            int Result = poll(&PollStruct, 1, TimeoutMS);

            if (Result < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }
            else if (Result == 0)
            {
                return 0;
            }

            return Value();
        }

        // TryAwait
    };
}
