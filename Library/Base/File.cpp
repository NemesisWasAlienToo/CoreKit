#pragma once

#include <iostream>
#include <string>
#include <unistd.h>

#include "Base/Descriptor.cpp"

namespace Core{
    class File : public Descriptor
    {
    private:
        /* data */
    public:
        enum TestType{
            CanRead = R_OK,
            CanWrite = W_OK,
            CanExecute = X_OK,
        };

        enum AccessType{
            Read,
            Create,
            Append,
            Write,
        };

        File() {}
        ~File() {}

        static inline bool Exist(const std::string& Name, int Tests = F_OK){
            return ( access( Name.c_str(), Tests ) != -1 );
        }
    };
}