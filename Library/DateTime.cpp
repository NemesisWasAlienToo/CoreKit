#pragma once

#include <iostream>
#include <ctime>

namespace Core
{
    class DateTime
    {
    private:
        int Day = 0;
        int Month = 0;
        int Year = 0;

    public:

        // Static functions

        static std::string Now()
        {
            time_t rawtime;
            struct tm *timeinfo;
            char buffer[40];

            time(&rawtime);
            timeinfo = localtime(&rawtime);

            strftime(buffer, 40, "%F %T", timeinfo);

            return buffer;
        }

        static std::string Test()
        {
            time_t rawtime = 0;
            struct tm *timeinfo;
            char buffer[40];

            // time(&rawtime);
            timeinfo = localtime(&rawtime);

            std::cout << timeinfo->tm_zone << std::endl;

            strftime(buffer, 40, "%F %T", timeinfo);

            return buffer;
        }

        static DateTime FromNow(){} // <---- Fix

        // Funtionality

        bool Epired(){ return false; } // <---- Fix
    };
}