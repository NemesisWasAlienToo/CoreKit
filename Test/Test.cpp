#include <iostream>
#include <string>
#include <thread>
#include <functional>

#include <Test.cpp>
#include <File.cpp>
#include <Directory.cpp>
#include <Iterable/List.cpp>
#include <Network/DNS.cpp>

using namespace Core;
using namespace std;

int main(int argc, char const *argv[])
{
    if(not Directory::Exist("opendir"))
    {
        Directory::Create("opendir");
        File::Create("./opendir/openfile");
    }

    Directory::ChangeDirectory("opendir");

    auto dir = Directory::Open(".");

    dir.Entries().ForEach([&](const Directory::Entry &entry)
                          { cout << entry.Name << endl; });

    dir.Close();

    return 0;
}