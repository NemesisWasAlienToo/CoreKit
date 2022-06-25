#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

#include <DateTime.hpp>

using namespace Core;

int main(int argc, char const *argv[])
{
    DateTime d = DateTime::Now();

    std::cout << d.ToString() << std::endl;
    std::cout << d.Format("%a, %d %b %Y %T %X Local", 120) << std::endl;
    std::cout << d.ToGMT().Format("%a, %d %b %Y %T %X GMT", 120) << std::endl;

    return 0;
}
