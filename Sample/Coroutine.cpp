#include <iostream>
#include <string>
#include <unistd.h>

#include <Coroutine.cpp>

using namespace Core;

#define async(Function)

class Co1 : public Coroutine<void(int)>
{
public:
    Co1(size_t stackSize = DefaultStackSize) : Coroutine(stackSize) {}

    void operator()(int a)
    {
        std::cout << "1 phase \n";

        sleep(1);

        Yield();

        std::cout << "2 phase \n";

        sleep(1);

        std::cout << "A is " << a << '\n';

        Yield();
    }
};

class Co2 : public Coroutine<int(int)>
{
public:
    Co2(size_t stackSize = DefaultStackSize) : Coroutine(stackSize) {}

    int operator()(int a)
    {
        Co1 coro(1024 * 4 * 4);

        coro.Start(1100);

        std::cout << "Between phases\n";

        coro.Continue();

        std::cout << "First stage finished\n";

        std::cout << "1 phase \n";

        sleep(1);

        Yield(1);

        std::cout << "2 phase \n";

        sleep(1);

        std::cout << "A is " << a << '\n';

        Yield(2);
    }
};

int main(int argc, char const *argv[])
{
    Co2 coro(1024 * 4 * 4);

    auto r = coro.Start(1100);

    std::cout << "Between phases \n";

    r = coro.Continue();

    std::cout << "Ended\n";
}