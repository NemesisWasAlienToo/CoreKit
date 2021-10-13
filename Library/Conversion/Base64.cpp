#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/engine.h>

namespace Core
{
    namespace Conversion
    {
        class Base64
        {
        private:
            Base64();

        public:
            static inline int PlainSize(int Size)
            {
                return 3 * Size / 4;
            }

            static inline int CypherSize(int Size)
            {
                return 4 * ((Size + 2) / 3);
            }
            
            static std::string From(const unsigned char *Data, int Size)
            {
                int _Size = 4 * ((Size + 2) / 3);
                char Output[_Size + 1];

                int Result = EVP_EncodeBlock(reinterpret_cast<unsigned char *>(Output), Data, Size);

                if (_Size != Result)
                {
                    std::cout << "Base64 String" << std::endl;
                    exit(-1);
                }

                return Output;
            }

            static int Bytes(const std::string &Base64String, unsigned char *Data)
            {
                int Result = EVP_DecodeBlock(Data, reinterpret_cast<const unsigned char *>(Base64String.c_str()), Base64String.length());
                
                if (Result != PlainSize(Base64String.length()))
                {
                    std::cout << "Base64 String" << std::endl;
                    exit(-1);
                }

                return Result;
            }
        };
    }
}
