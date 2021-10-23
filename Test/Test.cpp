#include <iostream>
#include <string>
#include <cstring>
#include <streambuf>
#include "Base/Test.cpp"
#include "Iterable/List.cpp"
#include "Iterable/Queue.cpp"

using namespace Core;
using namespace std;

int main(int argc, char const *argv[])
{
    Iterable::List<unsigned int> Numbers = Iterable::List<unsigned int>::Build(0, 9, [](size_t Index)
                                                                   { return Index; });

    Numbers.Remove(3);

    Numbers.Swap(1);

    Numbers.Add(2);

    Numbers.Add(20);

    std::cout << Numbers << std::endl;

    std::cout << Numbers.Where([](const unsigned int& Item){ return  Item > 4U;}) << std::endl;

    return 0;
}