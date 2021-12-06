#pragma once

// NOTICE : Compile with -ldl

#include <iostream>
#include <dlfcn.h>

namespace Core
{
    class DynamicLib
    {
    private:
        void *_Handle = nullptr;

    public:
        enum Flags
        {
            Lazy = RTLD_LAZY,
            Now = RTLD_NOW,
            Global = RTLD_GLOBAL,
            Local = RTLD_LOCAL,
            NoDelete = RTLD_NODELETE,
            NoLoad = RTLD_NOLOAD,
            DeepBind = RTLD_DEEPBIND,
        };

        DynamicLib(const std::string &Name, int Flags = 0)
        {
            _Handle = dlopen(Name.c_str(), Flags);

            if (!_Handle)
            {
                throw std::invalid_argument(dlerror());
            }
        }

        ~DynamicLib()
        {
            dlclose(_Handle);
        }

        // ### Static Functions

        template <typename T>
        static T *Next(const std::string &Name)
        {
            char *error = dlerror();
            auto ret = dlsym(RTLD_NEXT, Name.c_str());

            if ((error = dlerror()) != NULL)
            {
                throw std::invalid_argument(error);
            }

            return (T *)ret;
        }

        // ### Functionalities

        template <typename T>
        T *Symbol(const std::string &Name)
        {
            char *error = dlerror();
            auto ret = dlsym(_Handle, Name.c_str());

            if ((error = dlerror()) != NULL)
            {
                throw std::invalid_argument(error);
            }

            return (T *)ret;
        }
    };
}
