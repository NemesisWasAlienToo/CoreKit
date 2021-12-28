#pragma once

#include <iostream>
#include <time.h>

#include "Duration.cpp"

namespace Core
{
    class DateTime
    {
    private:
        struct timespec Spec;
        struct tm State;

    public:
        DateTime() = default;

        DateTime(timespec spec) : Spec(spec)
        {
            localtime_r(&spec.tv_sec, &State);
        }

        DateTime(timespec spec, struct tm state) : Spec(spec), State(state) {}

        // DateTime(int second, int minute, int hour, int, int day, int month, int year)
        // {
        //     State.tm_sec = second;
        // }

        DateTime(const DateTime &Other) : Spec(Other.Spec), State(Other.State) {}

        // Peroperties

        Duration Left()
        {
            struct timespec _Now, Diff;

            timespec_get(&_Now, TIME_UTC);

            Diff = {
                .tv_sec = Spec.tv_sec - _Now.tv_sec,
                .tv_nsec = Spec.tv_nsec - _Now.tv_nsec,
            };

            if (Diff.tv_sec < 0)
            {
                return {0, 1};
            }
            
            if (Diff.tv_sec == 0 && Diff.tv_nsec < 0)
            {
                return {0, 1};
            }

            if(Diff.tv_nsec < 0)
            {
                Diff.tv_sec--;
                Diff.tv_nsec += (long)10e8;
            }

            return Diff;
        }

        inline int Nanosecond() const
        {
            return Spec.tv_nsec;
        }

        inline void AddNanosecond(int nanosecond)
        {
            Spec.tv_nsec += nanosecond;
        }

        inline int Second() const
        {
            return State.tm_sec;
        }

        inline void AddSecond(int seconds)
        {
            State.tm_sec += seconds;

            Spec.tv_sec = mktime(&State);
        }

        inline int Minute() const
        {
            return State.tm_min;
        }

        inline void AddMinute(int minute)
        {
            State.tm_min += minute;

            Spec.tv_sec = mktime(&State);
        }

        inline int Hour() const
        {
            return State.tm_hour;
        }

        inline void AddHour(int hour)
        {
            State.tm_hour += hour;

            Spec.tv_sec = mktime(&State);
        }

        inline int Day() const
        {
            return State.tm_mday;
        }

        inline void AddDay(int day)
        {
            State.tm_mday += day;

            Spec.tv_sec = mktime(&State);
        }

        inline int Month() const
        {
            return State.tm_mon + 1;
        }

        inline void AddMonth(int month)
        {
            State.tm_mon += month;

            Spec.tv_sec = mktime(&State);
        }

        inline int Year() const
        {
            return State.tm_year + 1900;
        }

        inline void AddYear(int year)
        {
            State.tm_year += year;

            Spec.tv_sec = mktime(&State);
        }

        std::string ToString() const
        {
            char buffer[40];

            strftime(buffer, 40, "%F %T", &State);

            return buffer;
        }

        // Static functions

        static DateTime Now()
        {
            struct timespec rawtime;

            timespec_get(&rawtime, TIME_UTC);

            return DateTime(rawtime);
        }

        static DateTime FromNow(time_t Seconds)
        {
            auto Result = Now();

            Result.AddSecond(Seconds);

            return Result;
        }

        // Funtionality

        Duration operator-(const DateTime &Other) const
        {
            struct timespec Diff = {
                .tv_sec = Spec.tv_sec - Other.Spec.tv_sec,
                .tv_nsec = Spec.tv_nsec - Other.Spec.tv_nsec,
            };

            if (Diff.tv_sec < 0)
            {
                return {0, 1};
            }
            
            if (Diff.tv_sec == 0 && Diff.tv_nsec < 0)
            {
                return {0, 1};
            }

            if(Diff.tv_nsec < 0)
            {
                Diff.tv_sec--;
                Diff.tv_nsec += (long)10e8;
            }

            return Diff;
        }

        // DateTime operator+(const DateTime &Other) const

        DateTime &operator=(const DateTime &Other)
        {
            Spec = Other.Spec;
            State = Other.State;

            return *this;
        }

        bool operator==(const DateTime &Other) const
        {
            return Spec.tv_sec == Other.Spec.tv_sec && Spec.tv_nsec == Other.Spec.tv_nsec;
        }

        bool operator!=(const DateTime &Other) const
        {
            return Spec.tv_sec != Other.Spec.tv_sec || Spec.tv_nsec != Other.Spec.tv_nsec;
        }

        bool operator>(const DateTime &Other) const
        {
            if (Spec.tv_sec != Other.Spec.tv_sec)
            {
                return Spec.tv_sec > Other.Spec.tv_sec;
            }
            else
            {
                return Spec.tv_sec > Other.Spec.tv_sec;
            }
        }

        bool operator<(const DateTime &Other) const
        {
            if (Spec.tv_sec != Other.Spec.tv_sec)
            {
                return Spec.tv_sec < Other.Spec.tv_sec;
            }
            else
            {
                return Spec.tv_sec < Other.Spec.tv_sec;
            }
        }

        bool operator>=(const DateTime &Other) const
        {
            if (Spec.tv_sec != Other.Spec.tv_sec)
            {
                return Spec.tv_sec > Other.Spec.tv_sec;
            }
            else
            {
                return Spec.tv_sec >= Other.Spec.tv_sec;
            }
        }

        bool operator<=(const DateTime &Other) const
        {
            if (Spec.tv_sec != Other.Spec.tv_sec)
            {
                return Spec.tv_sec < Other.Spec.tv_sec;
            }
            else
            {
                return Spec.tv_sec <= Other.Spec.tv_sec;
            }
        }

        // Operators

        friend std::ostream &operator<<(std::ostream &os, const DateTime &dateTime)
        {
            return os << dateTime.ToString();
        }
    };
}