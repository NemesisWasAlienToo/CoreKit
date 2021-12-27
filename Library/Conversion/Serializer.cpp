#pragma once

#include <iostream>
#include <string>
#include <arpa/inet.h>

#include "Iterable/Queue.cpp"

#define NETWORK_BYTE_ORDER LITTLE_ENDIAN

namespace Core
{
    namespace Network
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

            Iterable::Queue<char>& Queue;

            // Constructors

            Serializer() = default;

            Serializer(Iterable::Queue<char>& queue) : Queue(queue) {}

            ~Serializer() = default;

            // Peroperties

            inline void Add(char *Data, size_t Size)
            {
                Queue.Add(Data, Size);
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

            Serializer &operator<<(const Iterable::Span<char>& Value)
            {
                Queue.Add(Value.Content(), Value.Length());

                return *this;
            }

            Serializer &operator<<(const std::string& Value)
            {
                Queue.Add(Value.c_str(), Value.length());

                return *this;
            }
        };
    }
}