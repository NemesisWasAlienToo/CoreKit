#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

namespace Core
{
    class Converter
    {
    private:
        Converter() {}
        ~Converter() {}

        static unsigned char Digit(char HexChar, bool Upper = false)
        {
            return HexChar - (HexChar > '9' ? ((Upper ? 'A' : 'a') - 10) : '0');
        }

        static unsigned char Number(const unsigned char Big, const unsigned char Small, bool Upper = false)
        {
            return ((Digit(Big) << 4) + Digit(Small));
        }

    public:
        static std::string Hex(const unsigned char *Data, size_t Size)
        {
            std::stringstream ss;

            for (size_t i = 0; i < Size; i++)
            {
                ss << std::hex << std::setw(2) << std::setfill('0') << (int)Data[i];
            }

            return ss.str();
        }

        static void Bytes(const std::string &HexString, unsigned char *Data, bool Upper = false)
        {
            size_t Len = HexString.length() / 2 ;
            const unsigned char *Str = (unsigned char *) HexString.c_str();

            for (size_t i = 0; i < Len; i++)
            {
                Data[i] = Number(*(Str), Str[1], Upper);
                Str += 2;
            }
        }

        // static std::string Base64(const unsigned char *Data, size_t Size)
        // {
        //     std::stringstream ss;

        //     for (size_t i = 0; i < Size; i++)
        //     {
        //         ss << std::hex << std::setw(2) << std::setfill('0') << (int)Data[i];
        //     }

        //     return ss.str();
        // }
    };
}
