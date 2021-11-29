#pragma once

#include <iostream>
#include <string>
#include <unistd.h>
#include <poll.h>
#include <sys/timerfd.h>
#include <sstream>
#include <iomanip>
#include <system_error>

#include "Base/Descriptor.cpp"

namespace Core
{
    struct Duration
    {
        time_t Seconds = 0;
        time_t Milliseconds = 0;
        time_t Microseconds = 0;
        time_t Nanoseconds = 0;

        Duration() = default;

        Duration(time_t seconds, time_t milliseconds = 0, time_t microseconds = 0, time_t nanoseconds = 0, bool periodic = false) : Seconds(seconds), Milliseconds(milliseconds), Microseconds(microseconds), Nanoseconds(nanoseconds) {}

        Duration(itimerspec ITimeSpec)
        {
            if (ITimeSpec.it_interval.tv_sec == 0 && ITimeSpec.it_interval.tv_nsec == 0)
            {
                Seconds = ITimeSpec.it_value.tv_sec;

                Nanoseconds = ITimeSpec.it_value.tv_nsec % 1000;
                ITimeSpec.it_value.tv_nsec /= 1000;

                Microseconds = ITimeSpec.it_value.tv_nsec % 1000;
                ITimeSpec.it_value.tv_nsec /= 1000;

                Nanoseconds = ITimeSpec.it_value.tv_nsec % 1000;
            }
            else
            {
                Seconds = ITimeSpec.it_interval.tv_sec;

                Nanoseconds = ITimeSpec.it_interval.tv_nsec % 1000;
                ITimeSpec.it_interval.tv_nsec /= 1000;

                Microseconds = ITimeSpec.it_interval.tv_nsec % 1000;
                ITimeSpec.it_interval.tv_nsec /= 1000;

                Nanoseconds = ITimeSpec.it_interval.tv_nsec % 1000;
            }
        }

        Duration(timespec TimeSpec)
        {
            Seconds = TimeSpec.tv_sec;

            Nanoseconds = TimeSpec.tv_nsec % 1000;
            TimeSpec.tv_nsec /= 1000;

            Microseconds = TimeSpec.tv_nsec % 1000;
            TimeSpec.tv_nsec /= 1000;

            Milliseconds = TimeSpec.tv_nsec % 1000;
        }

        Duration(time_t nanoseconds)
        {
            Nanoseconds = nanoseconds % 1000;
            nanoseconds /= 1000;

            Microseconds = nanoseconds % 1000;
            nanoseconds /= 1000;

            Milliseconds = nanoseconds % 1000;
            nanoseconds /= 1000;

            Seconds = nanoseconds / 1000;
        }

        time_t AsMilliseconds() const
        {
            return (Seconds * 1000) + Milliseconds;
        }

        time_t AsMicroseconds() const
        {
            return (Seconds * 1000000) + (Milliseconds * 1000) + Microseconds;
        }

        time_t AsNanoseconds() const
        {
            return (Seconds * 1000000000) + (Milliseconds * 1000000) + (Microseconds * 1000) + Nanoseconds;
        }

        bool IsZero() { return Seconds == 0 && Milliseconds == 0 && Microseconds == 0 && Nanoseconds == 00; }

        void Fill(timespec &TimeSpec)
        {
            TimeSpec.tv_sec = Seconds;
            TimeSpec.tv_nsec = (Milliseconds * 1000000) + (Microseconds * 1000) + Nanoseconds;
        }

        std::string ToString() const
        {
            std::stringstream ss;
            ss << Seconds << "." << std::setw(9) << std::setfill('0') << ((Milliseconds * 1000000) + (Microseconds * 1000) + Nanoseconds);
            return ss.str();
        }

        bool operator==(const Duration &Other)
        {
            return Seconds == Other.Seconds &&
                   Milliseconds == Other.Milliseconds &&
                   Microseconds == Other.Microseconds &&
                   Nanoseconds == Other.Nanoseconds;
        }

        bool operator!=(const Duration &Other)
        {
            return Seconds != Other.Seconds ||
                   Milliseconds != Other.Milliseconds ||
                   Microseconds != Other.Microseconds ||
                   Nanoseconds != Other.Nanoseconds;
        }

        bool operator<(const Duration &Other)
        {
            return Seconds < Other.Seconds &&
                   Milliseconds < Other.Milliseconds &&
                   Microseconds < Other.Microseconds &&
                   Nanoseconds < Other.Nanoseconds;
        }

        bool operator>(const Duration &Other)
        {
            return Seconds > Other.Seconds ||
                   Milliseconds > Other.Milliseconds ||
                   Microseconds > Other.Microseconds ||
                   Nanoseconds > Other.Nanoseconds;
        }

        bool operator<=(const Duration &Other)
        {
            return Seconds <= Other.Seconds &&
                   Milliseconds <= Other.Milliseconds &&
                   Microseconds <= Other.Microseconds &&
                   Nanoseconds <= Other.Nanoseconds;
        }

        bool operator>=(const Duration &Other)
        {
            return Seconds >= Other.Seconds ||
                   Milliseconds >= Other.Milliseconds ||
                   Microseconds >= Other.Microseconds ||
                   Nanoseconds >= Other.Nanoseconds;
        }

        friend std::ostream &operator<<(std::ostream &os, const Duration &duration)
        {
            return os << duration.ToString();
        }
    };

    class Timer : public Descriptor
    {
    public:
        enum TimerTypes
        {
            Realtime = CLOCK_REALTIME,
            RealtimeAlarm = CLOCK_REALTIME_ALARM,
            Boottime = CLOCK_BOOTTIME,
            BoottimeAlarm = CLOCK_BOOTTIME_ALARM,
            Monotonic = CLOCK_MONOTONIC,
        };

        enum TimerFlags
        {
            CloseOnExec = TFD_CLOEXEC,
            NonBlocking = TFD_NONBLOCK,
        };

        // ### Constructors

        Timer(int Handler) : Descriptor(Handler) {}

        Timer(TimerTypes Type, int Flags = 0)
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
            struct itimerspec _Duration = {0, 0};

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
