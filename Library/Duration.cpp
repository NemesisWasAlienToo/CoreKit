#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <sys/timerfd.h>

namespace Core
{
    struct Duration
    {
        time_t Seconds = 0;
        time_t Milliseconds = 0;
        time_t Microseconds = 0;
        time_t Nanoseconds = 0;

        Duration() = default;

        Duration(time_t seconds, time_t milliseconds = 0, time_t microseconds = 0, time_t nanoseconds = 0) : Seconds(seconds), Milliseconds(milliseconds), Microseconds(microseconds), Nanoseconds(nanoseconds) {}

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
    }
