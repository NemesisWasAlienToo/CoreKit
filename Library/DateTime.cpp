#pragma once

#include <iostream>
#include <time.h>

namespace Core
{
    class DateTime
    {
    private:
        time_t Time = 0;
        struct tm State;

    public:
        DateTime() = default;

        DateTime(time_t time) : Time(time)
        {
            localtime_r(&Time, &State);
        }

        DateTime(time_t time, struct tm state) : Time(time), State(state) {}

        // DateTime(int second, int minute, int hour, int, int day, int month, int year)
        // {
        //     State.tm_sec = second;
        // }

        DateTime(const DateTime &Other) : Time(Other.Time), State(Other.State) {}

        // Peroperties

        inline int Second() const
        {
            return State.tm_sec;
        }

        inline void AddSecond(int seconds)
        {
            State.tm_sec += seconds;

            Time = mktime(&State);
        }

        inline int Minute() const
        {
            return State.tm_min;
        }

        inline void AddMinute(int minute)
        {
            State.tm_min += minute;

            Time = mktime(&State);
        }

        inline int Hour() const
        {
            return State.tm_hour;
        }

        inline void AddHour(int hour)
        {
            State.tm_hour += hour;

            Time = mktime(&State);
        }

        inline int Day() const
        {
            return State.tm_mday;
        }

        inline void AddDay(int day)
        {
            State.tm_mday += day;

            Time = mktime(&State);
        }

        inline int Month() const
        {
            return State.tm_mon + 1;
        }

        inline void AddMonth(int month)
        {
            State.tm_mon += month;

            Time = mktime(&State);
        }

        inline int Year() const
        {
            return State.tm_year + 1900;
        }

        inline void AddYear(int year)
        {
            State.tm_year += year;

            Time = mktime(&State);
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
            time_t rawtime;
            time(&rawtime);

            return DateTime(rawtime);
        }

        static DateTime FromNow(time_t Seconds)
        {
            auto Result = Now();

            Result.AddSecond(Seconds);

            return Result;
        }

        // Funtionality

        bool Epired() const
        {
            return (*this) < Now();
        }

        double operator-(const DateTime &Other) const
        {
            return difftime(Time, Other.Time);
        }

        DateTime &operator=(const DateTime &Other)
        {
            Time = Other.Time;
            State = Other.State;

            return *this;
        }

        bool operator==(const DateTime &Other) const
        {
            return Time == Other.Time;
        }

        bool operator!=(const DateTime &Other) const
        {
            return Time != Other.Time;
        }

        bool operator>(const DateTime &Other) const
        {
            return Time > Other.Time;
        }

        bool operator<(const DateTime &Other) const
        {
            return Time < Other.Time;
        }

        bool operator>=(const DateTime &Other) const
        {
            return Time >= Other.Time;
        }

        bool operator<=(const DateTime &Other) const
        {
            return Time <= Other.Time;
        }

        // Operators

        friend std::ostream &operator<<(std::ostream &os, const DateTime &dateTime)
        {
            return os << dateTime.ToString();
        }
    };
}