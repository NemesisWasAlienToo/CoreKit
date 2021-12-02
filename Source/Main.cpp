// # Standard headers

#include <iostream>
#include <string>
#include <mutex>

// # User headers

#include "Iterable/List.cpp"
#include "Base/DateTime.cpp"

// # Usings

using namespace Core;

// # Global variables

int main(int argc, char const *argv[])
{
    std::cout << DateTime::Test() << std::endl;

    return 0;
}