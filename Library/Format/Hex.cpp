#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

#include <Iterable/Span.cpp>

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

            std::string From(const Iterable::Span<char> &Data)
            {
                std::stringstream ss;

                for (size_t i = 0; i < Data.Length(); i++)
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

            Iterable::Span<char> Bytes(const std::string &HexString, bool Upper = false)
            {
                Iterable::Span<char> Data(HexString.length() / 2);

                for (size_t i = 0; i < Data.Length(); i++)
                {
                    Data[i] = Number(HexString[2 * i], HexString[(2 * i) + 1], Upper);
                }

                return Data;
            }
        }
    }
}
