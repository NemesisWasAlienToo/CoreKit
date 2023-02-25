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

        constexpr Duration() = default;

        constexpr Duration(time_t seconds, time_t nanoseconds) : Seconds(seconds + (nanoseconds  / (long)1e9)), Nanoseconds(nanoseconds % (long)1e9) {}

        constexpr Duration(timespec TimeSpec) : Duration(TimeSpec.tv_sec, TimeSpec.tv_nsec) {}
        
        static constexpr Duration FromNanoseconds(time_t nanoseconds)
        {
            return Duration(nanoseconds / (long)1e9, nanoseconds % (long)1e9);
        }

        static constexpr Duration FromMilliseconds(time_t milliseconds)
        {
            return FromNanoseconds(milliseconds * (long)1e6);
        }

        static constexpr Duration FromMicroseconds(time_t microseconds)
        {
            return FromNanoseconds(microseconds * (long)1e3);
        }

        void AddNanoseconds(time_t Value)
        {
            Seconds += Value / (long)1e9;

            Value %= (long)1e9;

            Nanoseconds += Value;

            Seconds += Nanoseconds / (long)1e9;

            Nanoseconds %= (long)1e9;
        }

        void AddMilliseconds(time_t Value)
        {
            AddNanoseconds(Value * (long)1e6);
        }

        void AddMicroseconds(time_t Value)
        {
            AddNanoseconds(Value * (long)1e3);
        }

        inline constexpr time_t AsSeconds() const
        {
            return Seconds;
        }

        inline constexpr time_t AsMilliseconds() const
        {
            return (Seconds * (long)10e2) + (Nanoseconds / (long)1e6);
        }

        inline constexpr time_t AsMicroseconds() const
        {
            return (Seconds * (long)1e6) + (Nanoseconds / (long)10e2);
        }

        inline constexpr time_t AsNanoseconds() const
        {
            return (Seconds * (long)1e9) + Nanoseconds;
        }

        inline constexpr bool IsZero() { return Seconds == 0 && Nanoseconds == 00; }

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

        inline Duration operator-(const Duration &Other)
        {
            return FromNanoseconds(AsNanoseconds() - Other.AsNanoseconds());
        }

        inline Duration operator+(const Duration &Other)
        {
            return FromNanoseconds(AsNanoseconds() + Other.AsNanoseconds());
        }

        inline bool operator==(const Duration &Other)
        {
            return Seconds == Other.Seconds &&
                   Nanoseconds == Other.Nanoseconds;
        }

        inline bool operator!=(const Duration &Other)
        {
            return Seconds != Other.Seconds ||
                   Nanoseconds != Other.Nanoseconds;
        }

        inline bool operator<(const Duration &Other)
        {
            return Seconds < Other.Seconds ? true : Seconds == Other.Seconds && Nanoseconds < Other.Nanoseconds;
        }

        inline bool operator>(const Duration &Other)
        {
            return Seconds > Other.Seconds ? true : Seconds == Other.Seconds && Nanoseconds > Other.Nanoseconds;
        }

        inline bool operator<=(const Duration &Other)
        {
            return Seconds > Other.Seconds ? false : Seconds != Other.Seconds || Nanoseconds <= Other.Nanoseconds;
        }

        inline bool operator>=(const Duration &Other)
        {
            return Seconds < Other.Seconds ? false : Seconds != Other.Seconds || Nanoseconds >= Other.Nanoseconds;
        }

        friend std::ostream &operator<<(std::ostream &os, const Duration &duration)
        {
            return os << duration.ToString();
        }
    };
}
