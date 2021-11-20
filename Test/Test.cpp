#include <iostream>
#include <string>
#include <thread>
#include <functional>

#include "Base/Test.cpp"
#include "Base/File.cpp"
#include "Iterable/List.cpp"
#include "Iterable/Queue.cpp"
#include "Cryptography/Digest.cpp"
#include "Network/DNS.cpp"
#include "Network/DHT/Chord.cpp"

using namespace Core;
using namespace std;

void p1() { cout << "P1" << endl; }
void p2() { cout << "P2" << endl; }

int main(int argc, char const *argv[])
{
    Iterable::List<function<void(void)>> functions(2, false);

    functions.Add(p1);
    functions.Add(p2);

    functions.ForEach([](std::function<void (void)>& item){
        item();
    });

    return 0;
}