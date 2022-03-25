#pragma once

#include <iostream>
#include <string>
#include <type_traits>

#include <Descriptor.hpp>
#include <Iterable/Span.hpp>
#include <Iterable/List.hpp>
#include <Iterable/Queue.hpp>
#include <Network/EndPoint.hpp>
#include <Cryptography/Key.hpp>
#include <Network/DHT/Node.hpp>
#include <Network/Socket.hpp>

#ifndef NETWORK_BYTE_ORDER
#define NETWORK_BYTE_ORDER BIG_ENDIAN
#endif

// @todo Seperate serializer and deserializer IMPORTANT

namespace Core
{
    namespace Format
    {
        class Serializer
        {
        public:
#if BYTE_ORDER == NETWORK_BYTE_ORDER

            template <typename T>
            static std::void_t<std::enable_if<std::is_integral<T>::value>> static void Order(const T &Source, T &Destination)
            {
                Destination = Source;
            }
#else
            template <typename T>
            static std::void_t<std::enable_if<std::is_integral<T>::value>>
            Order(const T &Source, T &Destination)
            {
                const char *SourcePointer = (char *)&Source;
                char *DestinationPointer = (char *)&Destination;

                for (size_t i = 0; i < sizeof(T); i++)
                {
                    DestinationPointer[i] = SourcePointer[(sizeof(T) - 1) - i];
                }
            }
#endif
            template <typename T>
            static T Order(const T &Source)
            {
                T Res;

                Order(Source, Res);

                return Res;
            }

            // Public variables

            Iterable::Queue<char> &Queue;

            // Constructors

            Serializer(Iterable::Queue<char> &queue) : Queue(queue) {}

            Serializer(const Serializer &) = delete;

            ~Serializer() = default;

            // Peroperties

            inline size_t Length()
            {
                return Queue.Length();
            }

            void Realign()
            {
                Queue.Resize(Queue.Capacity());
            }

            void Clear()
            {
                Queue = Iterable::Queue<char>(Queue.Capacity());
            }

            void Clear(size_t NewSize)
            {
                Queue = Iterable::Queue<char>(NewSize);
            }

            inline Serializer &Add(const char *Data, size_t Size)
            {
                Queue.Add(Data, Size);

                return *this;
            }

            template <class T>
            inline Serializer &Add(const T &Object)
            {
                *this << Object;
                return *this;
            }

            template <class T>
            inline T Take()
            {
                T t;
                *this >> t;
                return t;
            }

            template <typename T>
            T &Modify(size_t Index)
            {
                char *Pointer = &Queue[Index];

                if (Queue.Capacity() == 0 || (static_cast<size_t>((&Queue.Content()[Queue.Capacity() - 1] - Pointer)) < sizeof(T)))
                    throw std::out_of_range("Size would access out of bound data");

                return *((T *)Pointer);
            }

            Iterable::Span<char> Dump()
            {
                Iterable::Span<char> Result(Queue.Length());

                Queue.Take(Result.Content(), Queue.Length());

                return Result;
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

            template <typename TValue>
            Serializer &operator<<(const Iterable::Iterable<TValue> &Value)
            {
                *this << Value.Length();

                Value.ForEach(
                    [this](const TValue &Item)
                    {
                        *this << Item;
                    });

                return *this;
            }

            Serializer &operator<<(const Iterable::Span<char> &Value)
            {
                *this << Value.Length();

                Queue.Add(Value.Content(), Value.Length());

                return *this;
            }

            Serializer &operator<<(const std::string &Value)
            {
                Queue.Add(Value.c_str(), Value.length() + 1);

                return *this;
            }

            Serializer &operator<<(const Cryptography::Key &Value)
            {
                *this << Value.Size;

                Queue.Add((char *)Value.Data, Value.Size);

                return *this;
            }

            Serializer &operator<<(const Network::Address &Value)
            {
                Network::Address::AddressFamily _Family = Order(Value.Family());

                Queue.Add((char *)&_Family, sizeof(_Family));

                Queue.Add((char *)Value.Content(), 16);

                return *this;
            }

            Serializer &operator<<(const Network::EndPoint &Value)
            {
                return *this << Value.address() << Order(Value.port()) << Order(Value.flow()) << Order(Value.scope());
            }

            Serializer &operator<<(const Network::DHT::Node &Value)
            {
                return *this << Value.Id << Value.EndPoint;
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

            template <typename TValue>
            Serializer &operator>>(Iterable::Iterable<TValue> &Value)
            {
                size_t Size = this->Take<size_t>();

                Value = Iterable::Iterable<TValue>(Size);

                // @todo Optimize when serializer and deserializer are seperated
                // Cuz we know there is no data inserted when taking data and
                // thus the queue hasn't wrapped around

                // if constexpr (std::is_integral_v<TValue>)

                for (size_t i = 0; i < Size; i++)
                {
                    Value.Add(this->Take<TValue>());
                }

                return *this;
            }

            Serializer &operator>>(Iterable::Span<char> &Value)
            {
                // @todo Optimize when serializer and deserializer are seperated
                // Cuz we know there is no data inserted when taking data and
                // thus the queue hasn't wrapped around

                Value = Iterable::Span<char>(this->Take<size_t>());

                for (size_t i = 0; i < Value.Length(); i++)
                {
                    Value[i] = Queue[i];
                }

                Queue.Free(Value.Length());

                return *this;
            }

            // @todo Optimize this

            Serializer &operator>>(std::string &Value)
            {
                while (Queue.Last() != '0')
                {
                    Value += Queue.Take();
                }

                Queue.Take();

                return *this;
            }

            Serializer &operator>>(Cryptography::Key &Value)
            {
                size_t Size = this->Take<size_t>();

                if (Value.Size != Size)
                    Value = Cryptography::Key(Size);

                Queue.Take((char *)Value.Data, Value.Size);

                return *this;
            }

            Serializer &operator>>(Network::Address &Value)
            {
                Network::Address::AddressFamily _Family;

                Queue.Take((char *)&_Family, sizeof(_Family));

                Value.Family() = Order(_Family);

                Queue.Take((char *)Value.Content(), 16);

                return *this;
            }

            Serializer &operator>>(Network::EndPoint &Value)
            {
                *this >> Value.address();
                Value.port(Order(this->Take<unsigned short>()));
                Value.flow(Order(this->Take<int>()));
                Value.scope(Order(this->Take<int>()));

                return *this;
            }

            Serializer &operator>>(Network::DHT::Node &Value)
            {
                return *this >> Value.Id >> Value.EndPoint;
            }

            friend std::ostream &operator<<(std::ostream &os, Serializer &Serializer)
            {
                while (!Serializer.Queue.IsEmpty())
                {
                    os << Serializer.Queue.Take();
                }

                return os;
            }

            friend Descriptor &operator<<(Descriptor &descriptor, Serializer &Serializer)
            {
                Serializer.Queue.Free(descriptor.Write(&Serializer.Queue.First(), Serializer.Queue.Length()));
                return descriptor;
            }

            friend Descriptor &operator>>(Descriptor &descriptor, Serializer &Serializer)
            {
                // @todo Optimize when NoWrap was implemented in iterable resize

                size_t size = descriptor.Received();
                char data[size];
                descriptor.Read(data, size);
                Serializer.Queue.Add(data, size);

                return descriptor;
            }

            Serializer &operator=(const Serializer &) = delete;
        };
    }
}