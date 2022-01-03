#pragma once

#include <iostream>
#include <string>

#include <Format/Hex.cpp>
#include <Cryptography/Random.cpp>

using namespace Core;

namespace Core
{
    namespace Network
    {
        namespace DHT
        {
            struct Key
            {
                // @todo maybr add endianness?

                // ### Constants

                size_t Size = 0;

                // ### Variables

                char *Data = nullptr;

                // ### Functions

                Key() = default;

                Key(size_t size, char Init = 0) : Size(size), Data(new char[size])
                {
                    Fill(Init);
                }

                Key(const std::string &Hex, size_t size) : Size(size), Data(new char[size])
                {
                    // @todo Optimize here

                    Fill(0);

                    Format::Hex::Bytes(Hex, &Data[Size - (Hex.length() / 2)]);
                }

                Key(const std::string &Hex) : Size(Hex.length() / 2), Data(new char[Size])
                {
                    Format::Hex::Bytes(Hex, Data);
                }

                Key(Key &&Other) : Size(Other.Size)
                {
                    std::swap(Data, Other.Data);
                }

                Key(const Key &Other) : Size(Other.Size), Data(new char[Size])
                {
                    for (size_t i = 0; i < Size; i++)
                    {
                        Data[i] = Other.Data[i];
                    }
                }

                Key(const char *data, size_t size) : Size(size), Data(new char[size])
                {
                    for (size_t i = 0; i < Size; i++)
                    {
                        Data[i] = data[i];
                    }
                }

                ~Key()
                {
                    delete[] Data;
                }

                // Statics

                static Key Generate(size_t size)
                {
                    Key Result(size);

                    Cryptography::Random::Load();

                    Cryptography::Random::Bytes((unsigned char *)Result.Data, size);

                    return Result;
                }

                // Functionalities

                void Fill(const char Init)
                {
                    for (size_t i = 0; i < Size; i++)
                    {
                        Data[i] = Init;
                    }
                }

                std::string ToString() const
                {
                    return Format::Hex::From(Data, Size);
                }

                Key Neighbor(size_t nth) const
                {
                    Key Result(Size);

                    Result.Set(nth);

                    return *this + Result;
                }

                size_t Critical() const // @todo Important Fix and optimize this
                {
                    size_t Index = 0;

                    Key key = Neighbor(1);

                    for (size_t i = 2; i <= Size; i++)
                    {
                        Key temp = Neighbor(i);
                        if( temp > key)
                        {
                            key = std::move(temp);
                            Index = i;
                        }
                    }

                    return Index;
                }

                bool Bit(size_t Number) const
                {
                    if (Number == 0)
                        throw std::invalid_argument("Zero th bit is meaningless");

                    Number--;

                    size_t Index = Number / 8;

                    size_t Shift = Number % 8;

                    return Data[(Size - 1) - Index] & (1 << Shift);
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
                    delete[] Data;
                    Data = new char[Size];

                    for (size_t i = 0; i < Size; i++)
                    {
                        Data[i] = Other.Data[i];
                    }

                    return *this;
                }

                char &operator[](size_t Index)
                {
                    return Data[Index];
                }

                const char &operator[](size_t Index) const
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

                    char Carry = 0;

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

                    char Carry = 1;

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
                    char Carry = 0;

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
                    char Carry = 1;

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

                // Key operator<<(size_t Count) const
                // {
                //     Key Result(Size);

                //     char Carry = 0;

                //     unsigned short Buffer = 0;

                //     for (int i = 0; i < Size; i++)
                //     {
                //         Buffer = Data[i] + Other.Data[i] + Carry;

                //         Result[i] = Buffer & 0xFF;

                //         Carry = Buffer >> 8;
                //     }

                //     return Result;
                // }

                // Key operator*(const Key &Other) = delete;
            };
        }
    }
}