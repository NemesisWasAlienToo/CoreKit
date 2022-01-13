#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

namespace Core
{
    namespace Format
    {
        namespace Hex
        {
            inline unsigned char Digit(char HexChar, bool Upper = false)
            {
                return HexChar - (HexChar > '9' ? ((Upper ? 'A' : 'a') - 10) : '0');
            }

            inline unsigned char Number(const unsigned char Big, const unsigned char Small, bool Upper = false)
            {
                return ((Digit(Big) << 4) + Digit(Small));
            }

            inline int PlainSize(int Size)
            {
                return Size / 2;
            }

            inline int CypherSize(int Size)
            {
                return 2 * Size;
            }

            template <typename T>
            std::string From(const T &Item)
            {
                const char *Data = (char *)&Item;
                std::stringstream ss;

                for (size_t i = 0; i < sizeof(Item); i++)
                {
                    // @ todo optimize this

                    ss << std::hex << std::setw(2) << std::setfill('0') << ((short)Data[i] & 0xff);
                }

                return ss.str();
            }

            std::string From(const char *Data, size_t Size)
            {
                std::stringstream ss;

                for (size_t i = 0; i < Size; i++)
                {
                    // @ todo optimize this

                    ss << std::hex << std::setw(2) << std::setfill('0') << ((short)Data[i] & 0xff);
                }

                return ss.str();
            }

            size_t Bytes(const std::string &HexString, char *Data, bool Upper = false)
            {
                size_t Len = HexString.length() / 2;

                for (size_t i = 0; i < Len; i++)
                {
                    Data[i] = Number(HexString[2 * i], HexString[(2 * i) + 1], Upper);
                }

                return Len;
            }
        }
    }
}
