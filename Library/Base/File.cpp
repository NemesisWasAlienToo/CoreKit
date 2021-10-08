#pragma once

#include <iostream>
#include <string>
#include <unistd.h>

namespace Core{
    class File
    {
    private:
        /* data */
    public:
        File(/* args */) {}
        ~File() {}

        static inline bool Exist(const std::string& Name){
            return ( access( Name.c_str(), F_OK ) != -1 );
        }
    };
}