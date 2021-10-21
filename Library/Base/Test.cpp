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

        // static constexpr char * Red = "";


    // Black: \033[30m
    // Red: \033[31m
    // Green: \033[32m
    // Yellow: \033[33m
    // Blue: \033[34m
    // Magenta: \033[35m
    // Cyan: \033[36m
    // White: \033[37m
    // Reset: \033[0m


        static void Assert(const std::string &Message, bool Result)
        {
            if (Result)
                std::cout << "\033[1;32mPassed\033[0m : " << Message << std::endl;
            else
                std::cout << "\033[1;31mFailed\033[0m : " << Message << std::endl;
        }

        // static void Warn(const std::string &Message, bool Result)
        // {
        //     if (Result)
        //         std::cout << "\033[1;32mPassed\033[0m : " << Message << std::endl;
        //     else
        //         std::cout << "\033[1;31mFailed\033[0m : " << Message << std::endl;
        // }

        template <typename T>
        static void Log(const T &Message)
        {
            time_t rawtime;
            struct tm *timeinfo;
            char buffer[80];

            time(&rawtime);
            timeinfo = localtime(&rawtime);

            strftime(buffer, 80, "%F %T", timeinfo);

            std::cout << "\033[1;35m"
                      << buffer
                      << "\033[0m : " << Message << std::endl;
        }
    };
}