#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <string_view>
#include <type_traits>

#include <Iterable/Span.hpp>

namespace Core::Format
{
    namespace Hex
    {
        inline unsigned char Digit(char HexChar, bool Upper = false)
        {
            return HexChar - (HexChar > '9' ? ((Upper ? 'A' : 'a') - 10) : '0');
        }

        inline unsigned char Number(const unsigned char Big, const unsigned char Small, bool Upper = false)
        {
            return ((Digit(Big, Upper) << 4) + Digit(Small, Upper));
        }

        inline int PlainSize(int Size)
        {
            return Size / 2;
        }

        inline int CypherSize(int Size)
        {
            return 2 * Size;
        }

        std::string From(const unsigned char *Data, size_t Size)
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

        size_t Bytes(std::string_view HexString, char *Data, bool Upper = false)
        {
            size_t Len = HexString.length() / 2;

            for (size_t i = 0; i < Len; i++)
            {
                Data[i] = Number(HexString[2 * i], HexString[(2 * i) + 1], Upper);
            }

            return Len;
        }

        Iterable::Span<char> Bytes(std::string_view HexString, bool Upper = false)
        {
            Iterable::Span<char> Data(HexString.length() / 2);

            for (size_t i = 0; i < Data.Length(); i++)
            {
                Data[i] = Number(HexString[2 * i], HexString[(2 * i) + 1], Upper);
            }

            return Data;
        }

        template <typename T>
        T To(std::string_view HexString, bool Upper = false)
        {
            T t = 0;
            size_t Length = std::min(sizeof(T) * 2, HexString.length());

            for (size_t i = 0; i < Length; i++)
            {
                t = t << 4;
                t += Digit(HexString[i], Upper);
            }

            return t;
        }
    }
}
