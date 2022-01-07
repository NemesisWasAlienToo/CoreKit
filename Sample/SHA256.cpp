#include <iostream>
#include <fstream>
#include <string>
#include <thread>

#include "Iterable/List.cpp"
#include "Cryptography/Digest.cpp"

using namespace Core;

int main(int argc, char const *argv[])
{
    if(argc < 2) {
        std::cout << "Enter filename" << std::endl;
        return 0;
    }

    std::string Line;
    std::string Content;

    std::ifstream MyReadFile(argv[1]);

    while (std::getline(MyReadFile, Line))
    {
        Content += Line;
    }

    MyReadFile.close();

    std::cout << Cryptography::SHA256::Hex(Content) << std::endl;

    return 0;
}
