#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <bitset>

namespace Core
{
    namespace Format
    {
        namespace Binary
        {
            inline int PlainSize(int Size)
            {
                return Size / 8;
            }

            inline int CypherSize(int Size)
            {
                return 8 * Size;
            }

            template <typename T>
            std::string From(const T &Item)
            {
                std::bitset<(sizeof(Item) * 8)> bin(Item);

                return bin.to_string();
            }

            std::string From(const char *Data, size_t Size)
            {
                std::stringstream ss;

                for (size_t i = 0; i < Size; i++)
                {
                    // @ todo optimize this

                    std::bitset<8> bin(Data[i]);

                    ss << bin;
                }

                return ss.str();
            }

            size_t Bytes(const std::string &BoolString, char *Data)
            {
                size_t Len = BoolString.length() / 8;

                for (size_t i = 0; i < Len; i++)
                {
                    char o = 0;

                    for (size_t j = 0; j < 8; j++)
                    {
                        if(BoolString[(i * 8) + j] == '1')
                        {
                            o |= 1 << (7 - j);
                        }
                    }

                    Data[i] = o;
                }

                return Len;
            }
        }
    }
}
