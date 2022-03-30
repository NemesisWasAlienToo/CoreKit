#include <iostream>
#include <string>
#include <thread>

#include <Alarm.hpp>
#include <Iterable/Span.hpp>

using namespace Core;

int main(int argc, char const *argv[])
{
    Alarm alarm;

    auto Pool = Iterable::Span<std::thread>(5);

    for (size_t i = 0; i < Pool.Length(); i++)
    {
        Pool[i] = std::thread(
            [&alarm]
            {
                while (true)
                {
                    alarm.Loop();
                }
            });
    }

    std::string cmd;

    while (true)
    {
        std::cin >> cmd;

        alarm.Schedual(
            {5, 0},
            []
            {
                Test::Log("Fired") << DateTime::Now() << "\n";
            });

        Test::Log("Schedualed") << DateTime::FromNow({5, 0}) << "\n";
    }

    Pool.ForEach(
        [](std::thread &Thread)
        {
            Thread.join();
        });

    return 0;
}