#pragma once

#include <iostream>
#include <ctime>

#include "DateTime.hpp"

namespace Core
{
    namespace Test
    {
        constexpr char *Black = (char *)"\033[30m";
        constexpr char *Red = (char *)"\033[31m";
        constexpr char *Green = (char *)"\033[32m";
        constexpr char *Yellow = (char *)"\033[33m";
        constexpr char *Blue = (char *)"\033[34m";
        constexpr char *Magenta = (char *)"\033[35m";
        constexpr char *Cyan = (char *)"\033[36m";
        constexpr char *White = (char *)"\033[37m";
        constexpr char *Reset = (char *)"\033[0m";

        void Assert(const std::string &Message, bool Result)
        {
            if (Result)
                std::cout << Green << "Passed" << Reset  << " : " << Message << std::endl;
            else
                std::cout << Red << "Failed" << Reset  << " : " << Message << std::endl;
        }

        std::string DateTime()
        {
            time_t rawtime;
            struct tm *timeinfo;
            char buffer[40];

            time(&rawtime);
            timeinfo = localtime(&rawtime);

            strftime(buffer, 40, "%F %T", timeinfo);

            return buffer;
        }

        std::ostream &Colored(const char *Color, const std::string &Title)
        {
            std::cout << Color
                      << "["
                      << DateTime::Now() << "] "
                      << Title << " : "
                      << Reset;

            std::cout.flush();

            return std::cout;
        }

        inline std::ostream &Error(const std::string &Title, std::ostream &os)
        {
            return Colored(Red, Title);
        }

        inline std::ostream &Error(const std::string &Title)
        {
            return Colored(Red, Title);
        }

        inline std::ostream &Warn(const std::string & Title)
        {
            return Colored(Yellow, Title);
        }

        inline std::ostream &Log(const std::string & Title)
        {
            return Colored(Green, Title);
        }
    };
}