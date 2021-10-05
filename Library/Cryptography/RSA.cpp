#pragma once

#include <openssl/rsa.h>

namespace Core
{
    namespace Cryptography
    {
        class RSA
        {
        public:
            // ## Types

            typedef ::RSA Key;

            // ## Functions

            RSA(/* args */);
            ~RSA();

        private:
            /* data */
        };
    }
}