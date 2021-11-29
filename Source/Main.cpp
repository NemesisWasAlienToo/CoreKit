// # Standard headers

#include <iostream>
#include <string>

// # User headers

#include "Iterable/List.cpp"
#include "Base/File.cpp"

// # Usings

using namespace Core;

// # Global variables

int main(int argc, char const *argv[])
{
    if (File::Exist("TestFile"))
        File::AppendAll("TestFile", "No hello cuz it exists.");
    else
        File::WriteAll("TestFile", "Hello there.\nHello there again.");

    std::cout << File::ReadAll("TestFile") << std::endl;

    return 0;
}