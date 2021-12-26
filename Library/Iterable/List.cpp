#pragma once

#include <iostream>
#include <sstream>
#include <functional>

#include "Iterable.cpp"

namespace Core
{
    namespace Iterable
    {
        template <typename T>
        class List : public Iterable<T>
        {
        protected:
            // ### Private Functions

            _FORCE_INLINE inline T &_ElementAt(size_t Index)
            {
                return this->_Content[Index];
            }

            _FORCE_INLINE inline const T &_ElementAt(size_t Index) const
            {
                return this->_Content[Index];
            }

        public:
            // ### Constructors

            List() : Iterable<T>() {}

            List(size_t Capacity, bool Growable = true) : Iterable<T>(Capacity, Growable) {}

            List(T *Array, int Count, bool Growable = true) : Iterable<T>(Array, Count, Growable) {}

            List(const List &Other) : Iterable<T>(Other) {}

            List(List &&Other) noexcept : Iterable<T>(std::move(Other)) {}

            // ### Destructor

            ~List() = default;

            // ### Functionalities

            std::string ToString()
            {
                std::stringstream ss;

                for (size_t i = 0; i < this->_Length; i++)
                {
                    ss << this->_Content[i] << '\n';
                }

                return ss.str();
            }

            // ### Static Functions

            static List<T> Build(size_t Start, size_t End, std::function<T(size_t)> Builder)
            {
                List<T> result((End - Start) + 1);

                for (size_t i = Start; i <= End; i++)
                {
                    result.Add(Builder(i));
                }

                return result;
            }

            // ### Operators

            List &operator=(const List &Other)
            {
                this->_Capacity = Other._Capacity;
                this->_Length = Other._Length;
                this->_Growable = Other._Growable;

                delete[] this->_Content;

                this->_Content = new T[Other._Capacity];

                for (size_t i = 0; i < Other._Length; i++)
                {
                    this->_Content[i] = Other._ElementAt(i);
                }

                return *this;
            }

            List &operator=(List &&Other) noexcept
            {
                if (this == &Other)
                    return *this;

                this->_Capacity = Other._Capacity;
                this->_Length = Other._Length;
                std::swap(this->_Content, Other._Content);

                return *this;
            }

            bool operator==(const List &Other) noexcept // Add more
            {
                return this->_Content == Other->_Content;
            }

            bool operator!=(const List &Other) noexcept
            {
                return this->_Content != Other->_Content;
            }

            // friend List<char> &operator<<(List<char> &list, const std::string &Message)
            // {
            //     list.Add(Message.c_str(), Message.length());

            //     return list;
            // }

            friend std::ostream &operator<<(std::ostream &os, const List &List)
            {
                // Check for << opeartor

                for (size_t i = 0; i < List._Length; i++)
                {
                    os << "[" << i << "] : " << List._ElementAt(i) << '\n';
                }

                return os;
            }
        };
    }
}