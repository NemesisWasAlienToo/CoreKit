// # Standard headers

#include <iostream>
#include <string>
#include <mutex>

// # User headers

#include "Iterable/List.cpp"
#include "DateTime.cpp"
#include "Timer.cpp"
#include "File.cpp"
#include "DynamicLib.cpp"

// # Usings

using namespace Core;

// # Global variables

int main(int argc, char const *argv[])
{
    std::cout << DateTime::Now() << std::endl;

    auto myout = File(STDIN_FILENO);

    for (size_t i = 0; i < 10; i++)
    {
        sleep(1);
        myout << 'a';
    }

    myout << '\n';

    return 0;
}