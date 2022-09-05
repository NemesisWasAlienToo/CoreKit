#pragma once

#include <iostream>
#include <string>
#include <type_traits>

#include <Descriptor.hpp>
#include <Iterable/Span.hpp>
#include <Iterable/List.hpp>
#include <Iterable/Queue.hpp>
#include <Network/EndPoint.hpp>

// @todo Separate serializer and deserializer IMPORTANT

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

        // Properties

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

        // @todo Implement
        // ReadAll for file
        // WriteAll for file
        // ReadLine
        // WriteLine

        ssize_t ReadOnce(Descriptor const &descriptor, size_t Length)
        {
            ssize_t Read = 0;
            struct iovec Vectors[2];
            Queue.IncreaseCapacity(Length);

            Read = descriptor.Read(Vectors, 1 + Queue.EmptyVectors(Vectors));

            Queue.AdvanceTail(Read);

            return Read;
        }

        ssize_t WriteOnce(Descriptor const &descriptor)
        {
            ssize_t Read = 0;
            struct iovec Vectors[2];

            Read = descriptor.Write(Vectors, 1 + Queue.DataVectors(Vectors));

            Queue.AdvanceTail(Read);

            return Read;
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

        // @todo Remove this after unifying iterable and span

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

        Stream &operator<<(std::string_view Value)
        {
            Queue.CopyFrom(Value.begin(), Value.length());

            return *this;
        }

        Stream &operator<<(const Network::Address &Value)
        {
            return *this << Value.ToString();
        }

        Stream &operator<<(const Network::EndPoint &Value)
        {
            return *this << Value.ToString();
        }

        friend Descriptor &operator<<(Descriptor &descriptor, Stream &Stream)
        {
            struct iovec Vectors[2];

            Stream.Queue.Free(descriptor.Write(Vectors, 1 + Stream.Queue.DataVectors(Vectors)));

            return descriptor;
        }

        friend Descriptor const &operator<<(Descriptor const &descriptor, Stream &Stream)
        {
            struct iovec Vectors[2];

            Stream.Queue.Free(descriptor.Write(Vectors, 1 + Stream.Queue.DataVectors(Vectors)));

            return descriptor;
        }

        friend Descriptor const &operator>>(Descriptor const &descriptor, Stream &Stream)
        {
            struct iovec Vectors[2];

            Stream.Queue.IncreaseCapacity(descriptor.Received());
            Stream.Queue.AdvanceTail(descriptor.Read(Vectors, Stream.Queue.EmptyVectors(Vectors) + 1));

            return descriptor;
        }

        friend Descriptor &operator>>(Descriptor &descriptor, Stream &Stream)
        {
            struct iovec Vectors[2];

            Stream.Queue.IncreaseCapacity(descriptor.Received());
            Stream.Queue.AdvanceTail(descriptor.Read(Vectors, Stream.Queue.EmptyVectors(Vectors) + 1));

            return descriptor;
        }

        Stream &operator=(const Stream &) = delete;
    };
}