#pragma once

#include <string>
#include <time.h>

#include <Duration.hpp>

namespace Core
{
    class DateTime
    {
    private:
        struct timespec Spec;
        struct tm State;

    public:
        enum class DaysOfWeek : unsigned short
        {
            Sunday = 0,
            Monday,
            Tuesday,
            Wednesday,
            Thursday,
            Friday,
            Saturday
        };

        enum class Months : unsigned short
        {
            January = 0,
            February,
            March,
            April,
            May,
            June,
            July,
            August,
            September,
            October,
            November,
            December
        };

        DateTime() = default;

        DateTime(timespec spec) : Spec(spec)
        {
            localtime_r(&spec.tv_sec, &State);
        }

        DateTime(timespec spec, struct tm state) : Spec(spec), State(state) {}

        DateTime(int year, int month = 0, int day = 0, int hour = 0, int minute = 0, int second = 0, long nanoseconds = 0)
        {
            State.tm_sec = second;
            State.tm_min = minute;
            State.tm_hour = hour;
            State.tm_mday = day;
            State.tm_mon = month;
            State.tm_year = year;

            Spec.tv_sec = mktime(&State);
            Spec.tv_nsec = nanoseconds;
        }

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

            if (Diff.tv_nsec < 0)
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

        void Add(const Duration &duration)
        {
            Spec.tv_sec += duration.Seconds;
            Spec.tv_nsec += duration.Nanoseconds;
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

        inline DaysOfWeek DayOfWeek() const
        {
            return DaysOfWeek(State.tm_wday);
        }

        inline void AddDay(int day)
        {
            State.tm_mday += day;

            Spec.tv_sec = mktime(&State);
        }

        inline Months Month() const
        {
            return Months(State.tm_mon);
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

        inline int DayOfYear() const
        {
            return State.tm_yday;
        }

        inline std::string Zone() const
        {
            return State.tm_zone;
        }

        std::string ToString() const
        {
            char buffer[40];

            strftime(buffer, 40, "%F %T", &State);

            return buffer;
        }

        // @todo Fix this

        std::string Format(char const *Format, size_t Size = 64) const
        {
            std::string retval;

            retval.resize(Size);
            size_t len = strftime((char *)retval.data(),
                                  retval.size(),
                                  Format,
                                  &State);

            retval.resize(len);

            return retval;
        }

        std::string Format(std::string const &Format) const
        {
            return this->Format(Format.c_str());
        }

        DateTime ToGMT() const
        {
            struct tm _State;

            gmtime_r(&Spec.tv_sec, &_State);

            return DateTime(Spec, _State);
        }

        // Static functions

        static DateTime Now()
        {
            struct timespec rawtime;

            timespec_get(&rawtime, TIME_UTC);

            return DateTime(rawtime);
        }

        static DateTime FromNow(time_t Seconds, time_t Nanoseconds = 0)
        {
            auto Result = Now();

            Result.AddSecond(Seconds);

            Result.AddNanosecond(Nanoseconds);

            return Result;
        }

        static DateTime FromNow(Duration const &duration)
        {
            return FromNow(duration.Seconds, duration.Nanoseconds);
        }

        // Funtionality

        bool IsExpired()
        {
            return *this <= Now();
        }

        // Operators

        Duration operator-(const DateTime &Other) const
        {
            struct timespec Diff = {
                .tv_sec = Spec.tv_sec - Other.Spec.tv_sec,
                .tv_nsec = Spec.tv_nsec - Other.Spec.tv_nsec,
            };

            if (Diff.tv_sec < 0)
            {
                return Diff;
            }

            if (Diff.tv_sec == 0 && Diff.tv_nsec < 0)
            {
                return Diff;
            }

            if (Diff.tv_nsec < 0)
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
            return Spec.tv_sec > Other.Spec.tv_sec ? true : Spec.tv_sec == Other.Spec.tv_sec && Spec.tv_nsec > Other.Spec.tv_nsec;
        }

        bool operator<(const DateTime &Other) const
        {
            return Spec.tv_sec < Other.Spec.tv_sec ? true : Spec.tv_sec == Other.Spec.tv_sec && Spec.tv_nsec < Other.Spec.tv_nsec;
        }

        bool operator>=(const DateTime &Other) const
        {
            return Spec.tv_sec < Other.Spec.tv_sec ? false : Spec.tv_sec != Other.Spec.tv_sec || Spec.tv_nsec >= Other.Spec.tv_nsec;
        }

        bool operator<=(const DateTime &Other) const
        {
            return Spec.tv_sec > Other.Spec.tv_sec ? false : Spec.tv_sec != Other.Spec.tv_sec || Spec.tv_nsec <= Other.Spec.tv_nsec;
        }

        // Operators

        friend std::ostream &operator<<(std::ostream &os, const DateTime &dateTime)
        {
            return os << dateTime.ToString();
        }
    };
}