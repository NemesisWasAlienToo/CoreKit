#pragma once

#include <iostream>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/engine.h>

namespace Core
{
    namespace Cryptography
    {
        class Random
        {
        protected:
            Random(/* args */) = default;
            ~Random();

        public:
            // ## Enums

            constexpr static char *urandom = (char *) "/dev/urandom";
            constexpr static char *arandom = (char *) "/dev/arandom";
            constexpr static char *random = (char *) "/dev/random";

            // ## static functions

            // static void Engine(); // Set engine mode to hardware or software

            // static int Needed(){ return ENTROPY_NEEDED; }

            // int RAND_bytes_ex(OSSL_LIB_CTX *ctx, unsigned char *buf, size_t num, unsigned int strength);

            static void Load(const char *File = NULL, long MaxSize = -1)
            {
                if (File == NULL)
                {
                    static bool Initiated = false;

                    if (!Initiated) // Not needed just to be sure
                    {
                        RAND_poll();
                        Initiated = true;
                    }
                }
                else
                {
                    int Result = RAND_load_file(File, MaxSize);

                    if (Result <= 0)
                    {
                        unsigned long Error = ERR_get_error();
                        std::cout << "Random Load : " << ERR_reason_error_string(Error) << std::endl;
                        exit(-1);
                    }
                }
            }

            static void Store(const char *File)
            {

                int Result = RAND_write_file(File);

                if (Result <= 0)
                {
                    unsigned long Error = ERR_get_error();
                    std::cout << "Random Load : " << ERR_reason_error_string(Error) << std::endl;
                    exit(-1);
                }
            }

            static void Seed(unsigned char *Data, int Size)
            {

                RAND_seed(Data, Size);
            }

            static void Add(unsigned char *Data, int Size, double Entropy)
            {
                RAND_add(Data, Size, Entropy);
            }

            static void Bytes(unsigned char *Data, int Size)
            {
                int Result = RAND_bytes(Data, Size);

                if (Result <= 0)
                {
                    unsigned long Error = ERR_get_error();
                    std::cout << "Random Bytes : " << ERR_reason_error_string(Error) << std::endl;
                    exit(-1);
                }
            }

            static void Hex(std::string &String, const int &Size)
            {

                unsigned char Data[Size];
                std::stringstream ss;

                Bytes(Data, Size);

                for (int i = 0; i < Size; i++)
                {
                    ss << std::hex << std::setw(2) << std::setfill('0') << (int)Data[i];
                }

                String.append(ss.str());
            }

            static std::string Hex(int Size)
            {

                unsigned char Data[Size];
                std::stringstream ss;

                Bytes(Data, Size);

                for (int i = 0; i < Size; i++)
                {
                    ss << std::hex << std::setw(2) << std::setfill('0') << (int)Data[i];
                }

                return ss.str();
            }
        };
    }
}