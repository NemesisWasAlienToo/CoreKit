#pragma once

#include <iostream>
#include <string>
#include <arpa/inet.h>

#include "Iterable/Queue.cpp"
#include "Network/EndPoint.cpp"
#include "Network/DHT/Key.cpp"
#include "Network/DHT/Node.cpp"

#ifndef NETWORK_BYTE_ORDER
#define NETWORK_BYTE_ORDER LITTLE_ENDIAN
#endif

// @todo Seperate serializer and deserializer IMPORTANT

namespace Core
{
    namespace Format
    {
        // @todo Add endianness maybe?

        class Serializer
        {
        private:
#if BYTE_ORDER == NETWORK_BYTE_ORDER

            template <typename T>
            void Order(const T &Source, T &Destination)
            {
                Destination = Source;
            }
#else

            template <typename T>
            void Order(const T &Source, T &Destination)
            {
                const void *SourcePointer = (void *)&Source;
                const void *DestinationPointer = (void *)&Destination;

                for (size_t i = 0; i < sizeof(T); i++)
                {
                    DestinationPointer[i] = SourcePointer[(sizeof(T) - 1) - i];
                }
            }
#endif

        public:
            // Public variables

            Iterable::Queue<char> &Queue;

            // Constructors

            Serializer() = default;

            Serializer(Iterable::Queue<char> &queue) : Queue(queue) {}

            ~Serializer() = default;

            // Peroperties

            inline size_t Length()
            {
                return Queue.Length();
            }

            inline Serializer &Add(char *Data, size_t Size)
            {
                Queue.Add(Data, Size);

                return *this;
            }

            template <typename T>
            T &Modify(size_t Index)
            {
                char *Pointer = &Queue[Index];

                if (Queue.Capacity() == 0 || (static_cast<size_t>((&Queue.Content()[Queue.Capacity() - 1] - Pointer)) < sizeof(T)))
                    throw std::out_of_range("Size would access out of bound data");

                return *((T *)Pointer);
            }

            // Input operators

            Serializer &operator<<(char Value)
            {
                Queue << Value;

                return *this;
            }

            Serializer &operator<<(short Value)
            {
                short _Value;

                Order(Value, _Value);

                Queue.Add((char *)&_Value, sizeof(_Value));

                return *this;
            }

            Serializer &operator<<(int Value)
            {
                int _Value;

                Order(Value, _Value);

                Queue.Add((char *)&_Value, sizeof(_Value));

                return *this;
            }

            Serializer &operator<<(long Value)
            {
                long _Value;

                Order(Value, _Value);

                Queue.Add((char *)&_Value, sizeof(_Value));

                return *this;
            }

            //

            Serializer &operator<<(unsigned char Value)
            {
                Queue << Value;

                return *this;
            }

            Serializer &operator<<(unsigned short Value)
            {
                unsigned short _Value;

                Order(Value, _Value);

                Queue.Add((char *)&_Value, sizeof(_Value));

                return *this;
            }

            Serializer &operator<<(unsigned int Value)
            {
                unsigned int _Value;

                Order(Value, _Value);

                Queue.Add((char *)&_Value, sizeof(_Value));

                return *this;
            }

            Serializer &operator<<(unsigned long Value)
            {
                unsigned long _Value;

                Order(Value, _Value);

                Queue.Add((char *)&_Value, sizeof(_Value));

                return *this;
            }

            //

            Serializer &operator<<(const Iterable::Span<char> &Value)
            {
                Queue.Add(Value.Content(), Value.Length());

                return *this;
            }

            Serializer &operator<<(const Network::DHT::Key &Value)
            {
                Queue.Add(Value.Data, Value.Size);

                return *this;
            }

            Serializer &operator<<(const std::string &Value)
            {
                Queue.Add(Value.c_str(), Value.length() + 1);

                return *this;
            }

            Serializer &operator<<(const Network::Address &Value)
            {
                auto str = Value.ToString();

                return *this << str;
            }

            Serializer &operator<<(const Network::EndPoint &Value)
            {
                auto str = Value.ToString();

                return *this << str;
            }

            Serializer &operator>>(const Network::DHT::Node::States &Value)
            {
                Queue.Add(static_cast<const char>(Value));
                return *this;
            }

            Serializer &operator<<(const Network::DHT::Node &Value)
            {
                return *this << Value.Id << Value.EndPoint << static_cast<const char>(Value.State);
            }

            Serializer &operator<<(Serializer &Value)
            {
                Queue.Add(Value.Queue);

                return *this;
            }

            // Output operators

            Serializer &operator>>(char &Value)
            {
                Queue >> Value;

                return *this;
            }

            Serializer &operator>>(short &Value)
            {
                short _Value;

                Queue.Take((char *)&_Value, sizeof(_Value));

                Order(_Value, Value);

                return *this;
            }

            Serializer &operator>>(int &Value)
            {
                int _Value;

                Queue.Take((char *)&_Value, sizeof(_Value));

                Order(_Value, Value);

                return *this;
            }

            Serializer &operator>>(long &Value)
            {
                long _Value;

                Queue.Take((char *)&_Value, sizeof(_Value));

                Order(_Value, Value);

                return *this;
            }

            //

            Serializer &operator>>(unsigned char &Value)
            {
                char *_value = (char *)&Value;

                Queue >> *_value;

                return *this;
            }

            Serializer &operator>>(unsigned short &Value)
            {
                unsigned short _Value;

                Queue.Take((char *)&_Value, sizeof(_Value));

                Order(_Value, Value);

                return *this;
            }

            Serializer &operator>>(unsigned int &Value)
            {
                unsigned int _Value;

                Queue.Take((char *)&_Value, sizeof(_Value));

                Order(_Value, Value);

                return *this;
            }

            Serializer &operator>>(unsigned long &Value)
            {
                unsigned long _Value;

                Queue.Take((char *)&_Value, sizeof(_Value));

                Order(_Value, Value);

                return *this;
            }

            // friend std::istream &operator>>(Serializer &Serializer, std::istream &is)
            // {
            //     while (!Serializer.Queue.IsEmpty())
            //     {
            //         os << Serializer.Queue.Take();
            //     }

            //     return os;
            // }

            //

            Serializer &operator>>(Iterable::Span<char> &Value)
            {
                // Apply byte order
                size_t Size = std::min(Value.Length(), Queue.Length());

                for (size_t i = 0; i < Size; i++)
                {
                    Value[i] = Queue[i];
                }

                Queue.Free(Size);

                return *this;
            }

            Serializer &operator>>(Network::DHT::Key &Value)
            {
                Queue.Take(Value.Data, Value.Size);

                return *this;
            }

            Serializer &operator>>(std::string &Value)
            {
                size_t Index;

                Queue.FirstWhere(Index, [](char &item) -> bool
                                 { return item == 0; });

                Value.resize(Index++);

                for (size_t i = 0; i < Index; i++)
                {
                    Value[i] = Queue[i];
                }

                Queue.Free(Index);

                return *this;
            }

            Serializer &operator>>(Network::Address &Value)
            {
                std::string str;

                *this >> str;

                Value = Network::Address(str);

                return *this;
            }

            Serializer &operator>>(Network::EndPoint &Value)
            {
                std::string str;

                *this >> str;

                Value = Network::EndPoint(str);

                return *this;
            }

            Serializer &operator>>(Network::DHT::Node::States &Value)
            {
                Value = static_cast<Network::DHT::Node::States>(Queue.Take());
                return *this;
            }

            Serializer &operator>>(Network::DHT::Node &Value)
            {
                return *this >> Value.Id >> Value.EndPoint >> Value.State;
            }

            // Serializer &operator>>(Serializer &Value)
            // {
            //     Queue.Add(Value.Queue);

            //     return *this;
            // }

            friend std::ostream &operator<<(std::ostream &os, Serializer &Serializer)
            {
                while (!Serializer.Queue.IsEmpty())
                {
                    os << Serializer.Queue.Take();
                }

                return os;
            }
        };
    }
}