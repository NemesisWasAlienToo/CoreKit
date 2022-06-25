#pragma once

#include <iostream>
#include <string>
#include <type_traits>

#include <Iterable/Span.hpp>
#include <Iterable/List.hpp>
#include <Iterable/Queue.hpp>

#ifndef NETWORK_BYTE_ORDER
#define NETWORK_BYTE_ORDER BIG_ENDIAN
#endif

// @todo Seperate serializer and deserializer IMPORTANT

namespace Core::Format
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
        static std::void_t<std::enable_if<std::is_integral_v<T>>>
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
            Queue.Realign();
        }

        void Clear()
        {
            Queue.Free();
        }

        inline Serializer &Add(const char *Data, size_t Size)
        {
            Queue.CopyFrom(Data, Size);

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
                throw std::out_of_range("Size would access out of bound memory");

            return *(reinterpret_cast<T *>(Pointer));
        }

        Iterable::Span<char> Dump()
        {
            Iterable::Span<char> Result(Queue.Length());

            Queue.MoveTo(Result.Content(), Queue.Length());

            return Result;
        }

        // Input operators

        template <typename T>
        std::enable_if_t<std::is_integral_v<T>, Serializer &>
        operator<<(T Value)
        {
            T _Value;

            Order(Value, _Value);

            Queue.CopyFrom((char *)&_Value, sizeof(_Value));

            return *this;
        }

        Serializer &operator<<(char Value)
        {
            Queue.Add(Value);

            return *this;
        }

        // @todo Remove this after unifiying iterable and span

        template <typename TValue>
        Serializer &operator<<(const Iterable::List<TValue> &Value)
        {
            *this << Value.Length();

            for (size_t i = 0; i < Value.Length(); i++)
            {
                *this << Value[i];
            }

            return *this;
        }

        template <typename TValue>
        Serializer &operator<<(const Iterable::Span<TValue> &Value)
        {
            *this << Value.Length();

            for (size_t i = 0; i < Value.Length(); i++)
            {
                *this << Value[i];
            }

            return *this;
        }

        Serializer &operator<<(const Iterable::Span<char> &Value)
        {
            *this << Value.Length();

            Queue.CopyFrom(Value.Content(), Value.Length());

            return *this;
        }

        Serializer &operator<<(const std::string &Value)
        {
            Queue.CopyFrom(Value.c_str(), Value.length() + 1);

            return *this;
        }

        // Output operators

        template <typename T>
        std::enable_if_t<std::is_integral_v<T>, Serializer &>
        operator>>(T &Value)
        {
            T _Value;

            Queue.MoveTo(reinterpret_cast<char *>(&_Value), sizeof(_Value));

            Order(_Value, Value);

            return *this;
        }

        Serializer &operator>>(char &Value)
        {
            Value = Queue.Take();

            return *this;
        }

        template <typename TValue>
        Serializer &operator>>(Iterable::List<TValue> &Value)
        {
            size_t Size = this->Take<size_t>();

            Value = Iterable::List<TValue>(Size);

            for (size_t i = 0; i < Size; i++)
            {
                Value.Add(this->Take<TValue>());
            }

            return *this;
        }

        template <typename TValue>
        Serializer &operator>>(Iterable::Span<TValue> &Value)
        {
            size_t Size = this->Take<size_t>();

            Value = Iterable::Span<TValue>(Size);

            // @todo Optimize when serializer and deserializer are seperated
            // Cuz we know there is no data inserted when taking data and
            // thus the queue hasn't wrapped around

            // if constexpr (std::is_integral_v<TValue>)

            for (size_t i = 0; i < Size; i++)
            {
                Value[i] = this->Take<TValue>();
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

        // @todo Optimize and fix this this

        Serializer &operator>>(std::string &Value)
        {
            while (!Queue.IsEmpty() && Queue.Tail() != '0')
            {
                Value += Queue.Take();
            }

            if (!Queue.IsEmpty())
                Queue.Take();

            return *this;
        }

        friend std::ostream &operator<<(std::ostream &os, Serializer &Serializer)
        {
            while (!Serializer.Queue.IsEmpty())
            {
                os << Serializer.Queue.Take();
            }

            return os;
        }

        friend std::istream &operator>>(std::istream &is, Serializer &Serializer)
        {
            std::string Inpt;

            is >> Inpt;

            Serializer.Add(Inpt.c_str(), Inpt.length());

            return is;
        }

        Serializer &operator=(const Serializer &) = delete;
    };
}