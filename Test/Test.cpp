#include <iostream>
#include <string>

#include "Base/Buffer.cpp"

using namespace std;

void Assert(const string &Message, bool Result)
{
    if (Result)
        cout << "\033[1;32mPassed\033[0m : " << Message << endl;
    else
        cout << "\033[1;31mFailed\033[0m : " << Message << endl;
}

int main(int argc, char const *argv[])
{
    Base::Buffer buf(2);
    char str;
    size_t count;

    Assert("Put 1", buf.Put('a'));
    Assert("Put 2", buf.Put('a'));

    count = buf.Length();

    Assert("Empty", buf.Skip(count) == count);

    Assert("Length", buf.Length() == 0);
    Assert("Take 1", !buf.Take(str));

    return 0;
}
