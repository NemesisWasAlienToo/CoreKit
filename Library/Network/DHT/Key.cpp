#pragma once

#include <iostream>
#include <string>

#include <Conversion/Hex.cpp>

using namespace Core;

namespace Core
{
    namespace Network
    {
        namespace DHT
        {
            struct Key
            {
                // ### Constants

                size_t Size = 0;

                // ### Variables

                unsigned char *Data = nullptr;

                // ### Functions

                Key() = default;

                Key(size_t size, unsigned char Init = 0) : Size(size), Data(new unsigned char[size])
                {
                    Fill(Init);
                }

                Key(const std::string &Hex, size_t size) : Size(size), Data(new unsigned char[size])
                {
                    // @todo Optimize here

                    Fill(0);

                    Conversion::Hex::Bytes(Hex, &Data[Size - (Hex.length() / 2)]);
                }

                Key(const std::string &Hex) : Size(Hex.length() / 2), Data(new unsigned char[Size])
                {
                    Conversion::Hex::Bytes(Hex, Data);
                }

                Key(Key &&Other) : Size(Other.Size)
                {
                    std::swap(Data, Other.Data);
                }

                Key(const Key &Other) : Size(Other.Size), Data(new unsigned char[Size])
                {
                    for (size_t i = 0; i < Size; i++)
                    {
                        Data[i] = Other.Data[i];
                    }
                }

                ~Key()
                {
                    delete[] Data;
                }

                void Fill(unsigned char Init)
                {
                    for (size_t i = 0; i < Size; i++)
                    {
                        Data[i] = Init;
                    }
                }

                std::string ToString()
                {
                    return Conversion::Hex::From(Data, Size);
                }

                Key Neighbor(size_t nth)
                {
                    Key Result(Size);

                    Result.Set(nth);

                    return *this + Result;
                }

                void Set(size_t Number)
                {
                    if (Number == 0)
                        return;

                    Number--;

                    size_t Index = Number / 8;

                    size_t Shift = Number % 8;

                    Data[(Size - 1) - Index] |= (1 << Shift);
                }

                void Reset(size_t Number)
                {
                    size_t Index = Number / 8;

                    size_t Shift = Number % 8;

                    Data[Index] &= ~(1 << Shift);
                }

                // ## Operators

                Key &operator=(Key &&Other)
                {
                    Size = Other.Size;

                    std::swap(Data, Other.Data);

                    return *this;
                }

                Key &operator=(const Key &Other)
                {
                    Size = Other.Size;

                    for (size_t i = 0; i < Size; i++)
                    {
                        Data[i] = Other.Data[i];
                    }

                    return *this;
                }

                unsigned char &operator[](size_t Index)
                {
                    return Data[Index];
                }

                const unsigned char &operator[](size_t Index) const
                {
                    return Data[Index];
                }

                bool operator>=(const Key &Other) const
                {
                    for (size_t i = 0; i < Size; i++)
                    {
                        if (Data[i] < Other.Data[i])
                            return false;
                    }

                    return true;
                }

                bool operator<=(const Key &Other) const
                {
                    for (size_t i = 0; i < Size; i++)
                    {
                        if (Data[i] > Other.Data[i])
                            return false;
                    }

                    return true;
                }

                bool operator==(const Key &Other) const
                {
                    for (size_t i = 0; i != Size; i++)
                    {
                        if (Data[i] != Other.Data[i])
                            return false;
                    }

                    return true;
                }

                bool operator!=(const Key &Other) const
                {
                    for (size_t i = 0; i != Size; i++)
                    {
                        if (Data[i] != Other.Data[i])
                            return true;
                    }

                    return false;
                }

                bool operator>(const Key &Other) const
                {
                    for (size_t i = 0; i < Size; i++)
                    {
                        if (Data[i] > Other.Data[i])
                            return true;
                        else if (Data[i] < Other.Data[i])
                            return false;
                    }

                    return false;
                }

                bool operator<(const Key &Other) const
                {
                    for (size_t i = 0; i < Size; i++)
                    {
                        if (Data[i] > Other.Data[i])
                            return false;
                        else if (Data[i] < Other.Data[i])
                            return true;
                    }

                    return false;
                }

                Key operator+(const Key &Other) const
                {
                    Key Result(Size);

                    unsigned char Carry = 0;

                    unsigned short Buffer = 0;

                    for (int i = Size - 1; i >= 0; i--)
                    {
                        Buffer = Data[i] + Other.Data[i] + Carry;

                        Result[i] = Buffer & 0xFF;

                        Carry = Buffer >> 8;
                    }

                    return Result;
                }

                Key operator~() const
                {
                    Key Result(Size);

                    for (int i = Size - 1; i >= 0; i--)
                    {
                        Result[i] = ~Data[i];
                    }

                    return Result;
                }

                Key operator-() const
                {
                    Key Result(Size);

                    unsigned char Carry = 1;

                    unsigned short Buffer = 0;

                    for (int i = Size - 1; i >= 0; i--)
                    {
                        Buffer = (~Data[i]) + Carry;

                        Result[i] = Buffer & 0xFF;

                        Carry = Buffer >> 8;
                    }

                    return Result;
                }

                Key operator-(const Key &Other) const
                {
                    return *this + (-Other);
                }

                Key &operator+=(const Key &Other)
                {
                    unsigned char Carry = 0;

                    unsigned short Buffer = 0;

                    for (size_t i = Size - 1; i >= 0; i--)
                    {
                        Buffer = Data[i] + Other.Data[i] + Carry;

                        Data[i] = Buffer & 0xFF;

                        Carry = Buffer >> 8;
                    }

                    return *this;
                }

                Key &operator-=(const Key &Other)
                {
                    unsigned char Carry = 1;

                    unsigned short Buffer = 0;

                    for (size_t i = Size - 1; i >= 0; i--)
                    {
                        Buffer = (~Data[i]) + Other.Data[i] + Carry;

                        Data[i] = Buffer & 0xFF;

                        Carry = Buffer >> 8;
                    }

                    return *this;
                }

                Key operator&(const Key &Other) const
                {
                    Key Result(Size);

                    for (int i = Size - 1; i >= 0; i--)
                    {
                        Result[i] = Data[i] & Other.Data[i];
                    }

                    return Result;
                }

                Key operator|(const Key &Other) const
                {
                    Key Result(Size);

                    for (int i = Size - 1; i >= 0; i--)
                    {
                        Result[i] = Data[i] | Other.Data[i];
                    }

                    return Result;
                }

                Key &operator&=(const Key &Other)
                {
                    for (size_t i = Size - 1; i >= 0; i--)
                    {
                        Data[i] &= Other.Data[i];
                    }

                    return *this;
                }

                Key &operator|=(const Key &Other)
                {
                    for (size_t i = Size - 1; i >= 0; i--)
                    {
                        Data[i] |= Other.Data[i];
                    }

                    return *this;
                }

                Key operator*(const Key &Other) = delete;
            };
        }
    }
}