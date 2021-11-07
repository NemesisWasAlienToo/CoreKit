#include <iostream>
#include <string>
#include <cstring>
#include <streambuf>
#include <thread>

#include "Base/Test.cpp"
#include "Base/File.cpp"
#include "Iterable/List.cpp"
#include "Iterable/Queue.cpp"
using namespace Core;
using namespace std;

int main(int argc, char const *argv[])
{
    string FileName = "TestFile";

    if (File::Exist(FileName))
    {
        File::Remove(FileName);
    }

    auto Permissions = File::OwnerAll | File::GroupAll | File::OtherAll;

    auto TestFile = File::Open("TestFile", File::ReadWrite | File::Append | File::CreateFile, Permissions);

    TestFile << "Hello there.\n"
             << "Hello again.";

    TestFile.Close();

    return 0;
}