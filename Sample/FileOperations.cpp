#include <iostream>
#include <string>
#include <cstring>

#include "Iterable/List.hpp"

#include "Descriptor.hpp"
#include "File.hpp"

using namespace Core;

int main(int argc, char const *argv[])
{
    if (File::Exist("testfile.txt"))
    {
        File f = File::Open("testfile.txt");

        std::cout << f.ReadLine() << std::endl;

        f.Close();

        File::Remove("testfile.txt");
    }
    else
    {
        File f = File::Create("testfile.txt");

        f.WriteLine("hello there");
    }

    return 0;
}
