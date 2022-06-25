#pragma once

#include <iostream>
#include <string>
#include <type_traits>

#include <Descriptor.hpp>
#include <Iterable/Span.hpp>
#include <Iterable/List.hpp>
#include <Iterable/Queue.hpp>
#include <Network/EndPoint.hpp>

// @todo Seperate serializer and deserializer IMPORTANT

namespace Core::Format
{
    class Stream
    {
    public:
        // Public variables

        Iterable::Queue<char> &Queue;

        // Constructors

        Stream(Iterable::Queue<char> &queue) : Queue(queue) {}

        Stream(const Stream &) = delete;

        // Peroperties

        // @todo Fix this

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

        inline Stream &Add(const char *Data, size_t Size)
        {
            Queue.CopyFrom(Data, Size);

            return *this;
        }

        template <class T>
        inline Stream &Add(const T &Object)
        {
            *this << Object;
            return *this;
        }

        // Input operators

        template <typename T>
        std::enable_if_t<std::is_integral_v<T>, Stream &>
        operator<<(T Value)
        {
            return *this << std::to_string(Value);
        }

        Stream &operator<<(char Value)
        {
            Queue.Add(Value);

            return *this;
        }

        // @todo Remove this after unifiying iterable and span

        template <typename TValue>
        Stream &operator<<(const Iterable::Span<TValue> &Value)
        {
            for (size_t i = 0; i < Value.Length(); i++)
            {
                *this << Value[i];
            }

            return *this;
        }

        Stream &operator<<(const Iterable::Span<char> &Value)
        {
            Queue.CopyFrom(Value.Content(), Value.Length());

            return *this;
        }

        template <size_t Size>
        Stream &operator<<(char const (&Value)[Size])
        {
            Queue.CopyFrom(Value, Size - 1);

            return *this;
        }

        Stream &operator<<(std::string const &Value)
        {
            Queue.CopyFrom(Value.c_str(), Value.length());

            return *this;
        }

        Stream &operator<<(std::string_view const &Value)
        {
            Queue.CopyFrom(Value.begin(), Value.length());

            return *this;
        }

        // @todo Add this

        // Stream &operator<<(std::string_view Value)
        // {
        //     Queue.CopyFrom(Value.begin(), Value.length());

        //     return *this;
        // }

        // Stream &operator<<(const Cryptography::Key &Value)
        // {
        //     auto str = Value.ToString();

        //     return *this << str;
        // }

        Stream &operator<<(const Network::Address &Value)
        {
            return *this << Value.ToString();
        }

        Stream &operator<<(const Network::EndPoint &Value)
        {
            return *this << Value.ToString();
        }

        friend std::ostream &operator<<(std::ostream &os, Stream &Stream)
        {
            while (!Stream.Queue.IsEmpty())
            {
                os << Stream.Queue.Take();
            }

            return os;
        }

        friend std::istream &operator>>(std::istream &is, Stream &Stream)
        {
            std::string Inpt;

            is >> Inpt;

            Stream.Add(Inpt.c_str(), Inpt.length());

            return is;
        }

        friend Descriptor &operator<<(Descriptor &descriptor, Stream &Stream)
        {
            size_t Sent = 0;
            struct iovec Vectors[2];

            do
            {
                Sent = descriptor.Write(Vectors, 1 + Stream.Queue.DataVector(Vectors));

                Stream.Queue.Free(Sent);
            } while (!Stream.Queue.IsEmpty() && Sent);

            return descriptor;
        }

        friend Descriptor const &operator<<(Descriptor const &descriptor, Stream &Stream)
        {
            size_t Sent = 0;
            struct iovec Vectors[2];

            do
            {
                Sent = descriptor.Write(Vectors, 1 + Stream.Queue.DataVector(Vectors));

                Stream.Queue.Free(Sent);
            } while (!Stream.Queue.IsEmpty() && Sent);

            return descriptor;
        }

        ssize_t ReadOnce(Descriptor const &descriptor, size_t Length)
        {
            size_t Read = 0;
            struct iovec Vectors[2];
            Queue.IncreaseCapacity(Length);

            Read = descriptor.Read(Vectors, 1 + Queue.EmptyVector(Vectors));

            Queue.AdvanceTail(Read);

            return Read;
        }

        // @todo Implement
        // ReadAll
        // WriteAll

        ssize_t WriteOnce(Descriptor const &descriptor)
        {
            size_t Read = 0;
            struct iovec Vectors[2];

            Read = descriptor.Write(Vectors, 1 + Queue.DataVector(Vectors));

            Queue.AdvanceTail(Read);

            return Read;
        }

        friend Descriptor const &operator>>(Descriptor const &descriptor, Stream &Stream)
        {
            size_t Read = 0;
            struct iovec Vectors[2];
            Stream.Queue.IncreaseCapacity(descriptor.Received());

            do
            {
                Read = descriptor.Read(Vectors, 1 + Stream.Queue.EmptyVector(Vectors));

                Stream.Queue.AdvanceTail(Read);

            } while (Read);

            // Read = descriptor.Read(Vectors, Stream.Queue.EmptyVector(Vectors) ? 2 : 1);

            // Stream.Queue.AdvanceTail(Read);

            return descriptor;
        }

        friend Descriptor &operator>>(Descriptor &descriptor, Stream &Stream)
        {
            size_t Read = 0;
            struct iovec Vectors[2];
            Stream.Queue.IncreaseCapacity(descriptor.Received());

            do
            {
                Read = descriptor.Read(Vectors, 1 + Stream.Queue.EmptyVector(Vectors));

                Stream.Queue.AdvanceTail(Read);

            } while (Read);

            // Read = descriptor.Read(Vectors, Stream.Queue.EmptyVector(Vectors) ? 2 : 1);

            // Stream.Queue.AdvanceTail(Read);

            return descriptor;
        }

        Stream &operator=(const Stream &) = delete;
    };
}