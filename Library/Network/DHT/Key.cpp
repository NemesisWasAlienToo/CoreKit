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

                unsigned char *Data = nullptr;

                // ### Functions

                Key() = default;

                Key(size_t size, char Init = 0) : Size(size), Data(new unsigned char[size])
                {
                    Fill(Init);
                }

                Key(const std::string &Hex, size_t size) : Size(size), Data(new unsigned char[size])
                {
                    // @todo Optimize here

                    Fill(0);

                    Format::Hex::Bytes(Hex, (char *)&Data[Size - (Hex.length() / 2)]);
                }

                Key(const std::string &Hex) : Size(Hex.length() / 2), Data(new unsigned char[Size])
                {
                    Format::Hex::Bytes(Hex, (char *)Data);
                }

                Key(Key &&Other)
                {
                    std::swap(Size, Other.Size);
                    std::swap(Data, Other.Data);
                }

                Key(const Key &Other) : Size(Other.Size), Data(new unsigned char[Other.Size])
                {
                    for (size_t i = 0; i < Size; i++)
                    {
                        Data[i] = Other.Data[i];
                    }
                }

                Key(const char *data, size_t size) : Size(size), Data(new unsigned char[size])
                {
                    for (size_t i = 0; i < Size; i++)
                    {
                        Data[i] = data[i];
                    }
                }

                ~Key()
                {
                    delete[] Data;
                    Data = nullptr;
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

                bool IsZero()
                {
                    for (size_t i = 0; i < Size; i++)
                    {
                        if (Data[i])
                        {
                            return false;
                        }
                    }

                    return true;
                }

                size_t MSNB()
                {
                    for (size_t i = 0; i < Size; i++)
                    {
                        if (Data[i])
                        {
                            for (size_t j = 0; j < 8; j++)
                            {
                                if (((Data[i] << j) & 0x80) != 0)
                                {
                                    return (((Size - i) * 8) - j);
                                }
                            }
                        }
                    }

                    return 0;
                }

                std::string ToString() const
                {
                    return Format::Hex::From((char *)Data, Size);
                }

                inline size_t NeighborCount() const
                {
                    return Size * 8;
                }

                Key Neighbor(size_t nth) const
                {
                    Key Result(Size);

                    Result.Set(nth);

                    // return *this + Result;

                    return std::move(Result += *this);
                }

                //
                size_t Critical() const // @todo Important Fix and optimize this
                {
                    size_t Index = 0;

                    Key key = Neighbor(1);

                    for (size_t i = 2; i <= NeighborCount(); i++)
                    {
                        Key Next = Neighbor(i);

                        if (Next > key)
                        {
                            key = std::move(Next);
                            Index = i;
                        }
                    }

                    return Index;
                }

                bool Bit(size_t Number) const
                {
                    if (Number == 0)
                        throw std::invalid_argument("Zero-th bit is meaningless");

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
                    std::swap(Size, Other.Size);
                    std::swap(Data, Other.Data);

                    return *this;
                }

                Key &operator=(const Key &Other)
                {
                    if (Size != Other.Size)
                    {
                        delete[] Data;
                        Size = Other.Size;
                        Data = new unsigned char[Other.Size];
                    }

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

                operator bool() const
                {
                    for (size_t i = 0; i != Size; i++)
                    {
                        if (Data[i])
                        {
                            return true;
                        }
                    }

                    return false;
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

                Key operator+(const size_t &Number) const
                {
                    size_t Other = Number;
                    Key Result(Size);

                    char Carry = 0;

                    unsigned short Buffer = 0;

                    for (int i = Size - 1; i >= 0; i--)
                    {
                        Buffer = Data[i] + (Other & 0xff) + Carry;

                        Result[i] = Buffer & 0xFF;

                        Carry = Buffer >> 8;
                        Other = Other >> 8;

                        if (Carry == 0 && Other == 0)
                        {
                            break;
                        }
                    }

                    return Result;
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
                        Buffer = ((unsigned char)(~Data[i])) + Carry;

                        Result.Data[i] = Buffer & 0xFF;

                        Carry = Buffer >> 8;
                    }

                    return Result;
                }

                // Key operator-(const Key &Other) const
                // {
                //     return *this + (-Other);
                // }

                Key operator-(const Key &Other) const
                {
                    Key Result(Size);

                    char Carry = 1;

                    unsigned short Buffer = 0;

                    for (int i = Size - 1; i >= 0; i--)
                    {
                        Buffer = Data[i] + ((unsigned char)(~Other.Data[i])) + Carry;

                        Result[i] = Buffer & 0xFF;

                        Carry = Buffer >> 8;
                    }

                    return Result;
                }

                Key operator+=(const size_t &Number)
                {
                    size_t Other = Number;
                    Key Result(Size);

                    char Carry = 0;

                    unsigned short Buffer = 0;

                    for (int i = Size - 1; i >= 0; i--)
                    {
                        Buffer = Data[i] + (Other & 0xff) + Carry;

                        Data[i] = Buffer & 0xFF;

                        Carry = Buffer >> 8;
                        Other = Other >> 8;

                        if (Carry == 0 && Other == 0)
                        {
                            break;
                        }
                    }

                    return Result;
                }

                Key &operator+=(const Key &Other)
                {
                    char Carry = 0;

                    unsigned short Buffer = 0;

                    for (int i = Size - 1; i >= 0; i--)
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

                    for (int i = Size - 1; i >= 0; i--)
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

                Key operator^(const Key &Other) const
                {
                    Key Result(Size);

                    for (int i = Size - 1; i >= 0; i--)
                    {
                        Result[i] = Data[i] ^ Other.Data[i];
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

                friend std::ostream &operator<<(std::ostream &os, const Key &key)
                {
                    return os << key.ToString();
                }

                Key &operator^=(const Key &Other)
                {
                    for (size_t i = Size - 1; i >= 0; i--)
                    {
                        Data[i] ^= Other.Data[i];
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