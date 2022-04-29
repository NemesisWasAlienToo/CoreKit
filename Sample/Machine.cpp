#include <iostream>
#include <unistd.h>

#include <Machine.hpp>

using namespace Core;

struct Switch1 : Machine<void(std::string)>
{
    void operator()()
    {
        CO_START;

        std::cout << "State 0 : " << Argument<0>() << std::endl;
        sleep(1);
        CO_YIELD();

        std::cout << "State 1" << std::endl;
        sleep(1);
        CO_YIELD();

        std::cout << "State 2" << std::endl;
        sleep(1);
        CO_TERMINATE();

        CO_END;
    }
};

int main(int argc, char const *argv[])
{
    std::string str = "abc";
    Switch1 s1;

    s1.Start(std::move(str));

    while (!s1.IsFinished())
    {
        std::cout << "Yielded" << std::endl;
        s1.Continue();
    }

    return 0;
}