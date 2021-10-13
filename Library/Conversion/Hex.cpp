#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/engine.h>

namespace Core
{
    namespace Conversion
    {
        class Hex
        {
        private:
            Hex() {}
            ~Hex() {}

            static inline unsigned char Digit(char HexChar, bool Upper = false)
            {
                return HexChar - (HexChar > '9' ? ((Upper ? 'A' : 'a') - 10) : '0');
            }

            static inline unsigned char Number(const unsigned char Big, const unsigned char Small, bool Upper = false)
            {
                return ((Digit(Big) << 4) + Digit(Small));
            }

        public:
            static inline int PlainSize(int Size)
            {
                return Size / 2;
            }

            static inline int CypherSize(int Size)
            {
                return 2 *  Size;
            }

            static std::string From(const unsigned char *Data, size_t Size)
            {
                std::stringstream ss;

                for (size_t i = 0; i < Size; i++)
                {
                    ss << std::hex << std::setw(2) << std::setfill('0') << (int)Data[i];
                }

                return ss.str();
            }

            static size_t Bytes(const std::string &HexString, unsigned char *Data, bool Upper = false)
            {
                size_t Len = HexString.length() / 2;
                const unsigned char *Str = (unsigned char *)HexString.c_str();

                for (size_t i = 0; i < Len; i++)
                {
                    Data[i] = Number(*(Str), Str[1], Upper);
                    Str += 2;
                }

                return Len;
            }
        };
    }
}
