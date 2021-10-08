#pragma once

#include <iostream>

namespace Core
{
    class Test
    {
    private:
        Test() {}
        ~Test() {}

    public:
        static void Assert(const std::string &Message, bool Result)
        {
            if (Result)
                std::cout << "\033[1;32mPassed\033[0m : " << Message << std::endl;
            else
                std::cout << "\033[1;31mFailed\033[0m : " << Message << std::endl;
        }
    };
}