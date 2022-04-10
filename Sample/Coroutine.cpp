#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

#include <Coroutine.hpp>

using namespace Core;

class Data
{
public:
    size_t Content = 0;
    Data() = default;
    Data(size_t Number) : Content(Number) {}
    Data(const Data &Other) : Content(Other.Content) {}
};

class Co : public Coroutine<int(Data)>
{
public:
    Co(size_t stackSize) : Coroutine(stackSize) {}

    void PrintStuff()
    {
        std::cout << "Stuff" << std::endl;

        Yield(1);
    }

    void operator()()
    {
        Argument<0>().Content += 100;

        std::string s = "f";

        std::cout << "1 phase \n";

        PrintStuff();

        std::cout << s << std::endl;

        Yield(2);

        Argument<0>().Content += 100;

        std::cout << "2 phase \n";

        s = std::string("hello");

        Terminate(3);
    }
};

int main(int argc, char const *argv[])
{
    Co coro(1024 * 4 * 4);

    Data a{100};

    // std::cout << coro.Start(a) << std::endl;

    coro.Start(a);

    while (!coro.IsFinished())
    {
        sleep(1);

        // std::cout << "Yielded : " << coro.Continue() << std::endl;

        coro.Continue();
    }

    std::cout << "Ended\n";

    Data b{100};

    std::cout << coro.Start(b) << std::endl;

    // coro.Start(b);

    while (!coro.IsFinished())
    {
        sleep(1);

        std::cout << "Yielded : " << coro.Continue() << std::endl;

        // coro.Continue();
    }

    std::cout << "A is " << b.Content << '\n';
}