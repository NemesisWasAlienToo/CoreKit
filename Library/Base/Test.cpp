#pragma once

#include <iostream>
#include <ctime>

namespace Core
{
    class Test
    {
    private:
        Test() {}
        ~Test() {}

    public:
        static constexpr char *Black = (char *)"\033[30m";
        static constexpr char *Red = (char *)"\033[31m";
        static constexpr char *Green = (char *)"\033[32m";
        static constexpr char *Yellow = (char *)"\033[33m";
        static constexpr char *Blue = (char *)"\033[34m";
        static constexpr char *Magenta = (char *)"\033[35m";
        static constexpr char *Cyan = (char *)"\033[36m";
        static constexpr char *White = (char *)"\033[37m";
        static constexpr char *Reset = (char *)"\033[0m";

        static void Assert(const std::string &Message, bool Result)
        {
            if (Result)
                std::cout << "\033[1;32mPassed\033[0m : " << Message << std::endl;
            else
                std::cout << "\033[1;31mFailed\033[0m : " << Message << std::endl;
        }

        static std::string DateTime()
        {
            time_t rawtime;
            struct tm *timeinfo;
            char buffer[40];

            time(&rawtime);
            timeinfo = localtime(&rawtime);

            strftime(buffer, 40, "%F %T", timeinfo);

            return buffer;
        }

        static std::ostream &Error()
        {
            std::cout << Red
                      << DateTime() << " : "
                      << Reset;

            return std::cout;
        }

        static std::ostream &Warn()
        {
            std::cout << Yellow
                      << DateTime() << " : "
                      << Reset;

            return std::cout;
        }

        static std::ostream &Log()
        {
            std::cout << Green
                      << DateTime() << " : "
                      << Reset;

            return std::cout;
        }
    };
}