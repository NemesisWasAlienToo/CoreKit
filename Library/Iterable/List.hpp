#pragma once

#include <iostream>
#include <sstream>
#include <functional>

#include "Iterable.hpp"

namespace Core
{
    namespace Iterable
    {
        template <typename T>
        class List : public Iterable<T>
        {
        protected:
            // ### Private Functions

            inline T &_ElementAt(size_t Index) override
            {
                return this->_Content[Index];
            }

            inline const T &_ElementAt(size_t Index) const override
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

            List &operator=(List &&Other) noexcept = default;

            List &operator=(const List &Other) = default;

            bool operator!=(const List &Other) noexcept
            {
                return this->_Content != Other->_Content;
            }

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