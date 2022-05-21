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
        class Stringifier
        {
        public:
            // Public variables

            Iterable::Queue<char> &Queue;

            // Constructors

            Stringifier(Iterable::Queue<char> &queue) : Queue(queue) {}

            Stringifier(const Stringifier &) = delete;

            ~Stringifier() = default;

            // Peroperties

            std::string ToString() const
            {
                return std::string(Queue.Content(), Queue.Length());
            }

            std::string_view ToStringView() const
            {
                return std::string_view(Queue.Content(), Queue.Length());
            }

            void Clear()
            {
                Queue = Iterable::Queue<char>(Queue.Capacity());
            }

            void Clear(size_t NewSize)
            {
                Queue = Iterable::Queue<char>(NewSize);
            }

            inline Stringifier &Add(const char *Data, size_t Size)
            {
                Queue.Add(Data, Size);

                return *this;
            }

            template <class T>
            inline Stringifier &Add(const T &Object)
            {
                *this << Object;
                return *this;
            }

            // Input operators

            template <typename T>
            std::enable_if_t<std::is_integral_v<T>, Stringifier &>
            operator<<(T Value)
            {
                return *this << std::to_string(Value);
            }

            Stringifier &operator<<(char Value)
            {
                Queue.Add(Value);

                return *this;
            }

            template <typename TValue>
            Stringifier &operator<<(const Iterable::Iterable<TValue> &Value)
            {
                for (size_t i = 0; i < Value.Length(); i++)
                {
                    *this << Value[i];
                }

                return *this;
            }

            // @todo Remove this after unifiying iterable and span

            template <typename TValue>
            Stringifier &operator<<(const Iterable::Span<TValue> &Value)
            {
                for (size_t i = 0; i < Value.Length(); i++)
                {
                    *this << Value[i];
                }

                return *this;
            }

            Stringifier &operator<<(const Iterable::Span<char> &Value)
            {
                Queue.Add(Value.Content(), Value.Length());

                return *this;
            }

            Stringifier &operator<<(const std::string &Value)
            {
                Queue.Add(Value.c_str(), Value.length());

                return *this;
            }

            Stringifier &operator<<(const Cryptography::Key &Value)
            {
                auto str = Value.ToString();

                return *this << str;
            }

            Stringifier &operator<<(const Network::Address &Value)
            {
                return *this << Value.ToString();
            }

            Stringifier &operator<<(const Network::EndPoint &Value)
            {
                return *this << Value.ToString();
            }

            Stringifier &operator<<(Stringifier &Value)
            {
                Queue.Add(Value.Queue);

                return *this;
            }

            friend std::ostream &operator<<(std::ostream &os, Stringifier &Stringifier)
            {
                while (!Stringifier.Queue.IsEmpty())
                {
                    os << Stringifier.Queue.Take();
                }

                return os;
            }

            friend std::istream &operator>>(std::istream &is, Stringifier &Stringifier)
            {
                std::string Inpt;

                is >> Inpt;

                Stringifier.Add(Inpt.c_str(), Inpt.length());

                return is;
            }

            friend Descriptor &operator<<(Descriptor &descriptor, Stringifier &Stringifier)
            {
                auto [Pointer, Size] = Stringifier.Queue.Chunk();

                Stringifier.Queue.Free(descriptor.Write(Pointer, Size));
                return descriptor;
            }

            friend Descriptor const &operator<<(Descriptor const &descriptor, Stringifier &Stringifier)
            {
                auto [Pointer, Size] = Stringifier.Queue.Chunk();

                Stringifier.Queue.Free(descriptor.Write(Pointer, Size));
                return descriptor;
            }

            friend Descriptor const &operator>>(Descriptor const &descriptor, Stringifier &Stringifier)
            {
                // @todo Optimize when NoWrap was implemented in iterable resize

                size_t size = descriptor.Received();
                char data[size];
                descriptor.Read(data, size);
                Stringifier.Queue.Add(data, size);

                return descriptor;
            }
            friend Descriptor &operator>>(Descriptor &descriptor, Stringifier &Stringifier)
            {
                // @todo Optimize when NoWrap was implemented in iterable resize

                size_t size = descriptor.Received();
                char data[size];
                descriptor.Read(data, size);
                Stringifier.Queue.Add(data, size);

                return descriptor;
            }

            Stringifier &operator=(const Stringifier &) = delete;
        };
    }
}