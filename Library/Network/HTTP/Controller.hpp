#pragma once

#include <iostream>
#include <string>
#include <regex>
#include <map>
#include <cxxabi.h>

#include <Network/HTTP/Response.hpp>
#include <Network/HTTP/Request.hpp>
#include <Iterable/List.hpp>
#include <Iterable/Span.hpp>

namespace Core
{
    namespace Network
    {
        namespace HTTP
        {
            struct Action
            {
            };

            class Controller
            {
                std::string _Name;

            public:
                Controller()
                {
                    char *demangled = abi::__cxa_demangle(typeid(*this).name(), 0, 0, nullptr);
                    _Name = demangled;
                    free(demangled);
                }

                const std::string& Name() const
                {
                    return _Name;
                }

            protected:
            };
        }
    }
}