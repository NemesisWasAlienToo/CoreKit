#include <iostream>
#include <string>
#include <unistd.h>

#include <Coroutine.cpp>

using namespace Core;

class Co : public Coroutine<int(int&)>
{
public:
    Co(size_t stackSize = DefaultStackSize) : Coroutine(stackSize) {}

    void PrintStuff()
    {
        std::cout << "Stuff" << std::endl;

        Yield(4);

        std::cout << "Stuff 2" << std::endl;
    }

    int operator()(int& a)
    {
        a += 100;

        std::cout << "1 phase \n";

        PrintStuff();

        Yield(2);

        a += 100;

        std::cout << "2 phase \n";

        Terminate(3);
    }
};

int main(int argc, char const *argv[])
{
    Co coro(1024 * 4 * 4);

    int a = 100;

    auto r = coro.Start(a);

    while (!coro.IsFinished())
    {
        sleep(1);
        
        r = coro.Continue();
    }

    std::cout << "Ended\n";

    std::cout << "A is " << a << '\n';
}