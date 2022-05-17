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
        time_t Nanoseconds = 0;

        Duration() = default;

        Duration(time_t seconds, time_t nanoseconds = 0) : Seconds(seconds), Nanoseconds(nanoseconds) {}

        Duration(itimerspec ITimeSpec)
        {
            if (ITimeSpec.it_interval.tv_sec == 0 && ITimeSpec.it_interval.tv_nsec == 0)
            {
                Seconds = ITimeSpec.it_value.tv_sec;

                Nanoseconds = ITimeSpec.it_value.tv_nsec;
            }
            else
            {
                Seconds = ITimeSpec.it_interval.tv_sec;

                Nanoseconds = ITimeSpec.it_interval.tv_nsec;
            }
        }

        Duration(timespec TimeSpec)
        {
            Seconds = TimeSpec.tv_sec + (TimeSpec.tv_nsec / (long)1e9);

            Nanoseconds = TimeSpec.tv_nsec % (long)1e9;
        }

        Duration(time_t nanoseconds)
        {
            Seconds = nanoseconds / (long)1e9;

            Nanoseconds = nanoseconds % (long)1e9;
        }

        static Duration FromMilliseconds(size_t milliseconds)
        {
            return Duration(milliseconds / 1000, (milliseconds % 1000) * 1e6);
        }

        static Duration FromMicroseconds(size_t microseconds)
        {
            return Duration(microseconds / 1000'000, (microseconds % 1000) * 1e3);
        }

        void AddMilliseconds(time_t Value)
        {
            Seconds += Value / (long)1e3;

            Value %= (long)1e3;

            Nanoseconds += (Value * (long)1e6);

            Seconds += Nanoseconds / (long)1e9;

            Nanoseconds %= (long)1e9;
        }

        void AddMicroseconds(time_t Value)
        {
            Seconds += Value / (long)1e6;

            Value %= (long)1e6;

            Nanoseconds += (Value * (long)10e2);

            Seconds += Nanoseconds / (long)1e9;

            Nanoseconds %= (long)1e9;
        }

        time_t AsMilliseconds() const
        {
            return (Seconds * (long)10e2) + (Nanoseconds / (long)1e6);
        }

        time_t AsMicroseconds() const
        {
            return (Seconds * (long)1e6) + (Nanoseconds / (long)10e2);
        }

        time_t AsNanoseconds() const
        {
            return (Seconds * (long)1e9 ) +  Nanoseconds;
        }

        bool IsZero() { return Seconds == 0 && Nanoseconds == 00; }

        void Fill(timespec &TimeSpec)
        {
            TimeSpec.tv_sec = Seconds;
            TimeSpec.tv_nsec = Nanoseconds;
        }

        std::string ToString() const
        {
            std::stringstream ss;
            ss << Seconds << "." << std::setw(9) << std::setfill('0') << Nanoseconds;
            return ss.str();
        }

        bool operator==(const Duration &Other)
        {
            return Seconds == Other.Seconds &&
                   Nanoseconds == Other.Nanoseconds;
        }

        bool operator!=(const Duration &Other)
        {
            return Seconds != Other.Seconds ||
                   Nanoseconds != Other.Nanoseconds;
        }

        bool operator<(const Duration &Other)
        {
            return Seconds < Other.Seconds ? true : Seconds == Other.Seconds && Nanoseconds < Other.Nanoseconds;
        }

        bool operator>(const Duration &Other)
        {
            return Seconds > Other.Seconds ? true : Seconds == Other.Seconds && Nanoseconds > Other.Nanoseconds;
        }

        bool operator<=(const Duration &Other)
        {
            return Seconds > Other.Seconds ? false : Seconds != Other.Seconds || Nanoseconds <= Other.Nanoseconds;
        }

        bool operator>=(const Duration &Other)
        {
            return Seconds < Other.Seconds ? false : Seconds != Other.Seconds || Nanoseconds >= Other.Nanoseconds;
        }

        friend std::ostream &operator<<(std::ostream &os, const Duration &duration)
        {
            return os << duration.ToString();
        }
    };
    }
