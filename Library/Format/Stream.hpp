#pragma once

#include <iostream>
#include <string>
#include <type_traits>

#include <Iterable/Span.hpp>
#include <Iterable/List.hpp>
#include <Iterable/Queue.hpp>
#include <Network/EndPoint.hpp>

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

        // @todo Remove this from here

        Stream &operator<<(const Network::Address &Value)
        {
            return *this << Value.ToString();
        }

        Stream &operator<<(const Network::EndPoint &Value)
        {
            return *this << Value.ToString();
        }

        Stream &operator=(const Stream &) = delete;
    };
}