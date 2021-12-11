#include <iostream>
#include <string>
#include <thread>
#include <functional>

#include <Test.cpp>
#include <File.cpp>
#include <Iterable/List.cpp>
#include <Network/DNS.cpp>

using namespace Core;
using namespace std;

int main(int argc, char const *argv[])
{
    auto dir = File::Open("opendir", File::Directory);

    dir.Close();

    return 0;
}