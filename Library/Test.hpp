#pragma once

#include <iostream>
#include <DateTime.hpp>
#include <File.hpp>

namespace Core
{
    namespace Test
    {
        constexpr char *Reset = (char *)"\033[0m";
        constexpr char *Black = (char *)"\033[30m";
        constexpr char *Red = (char *)"\033[31m";
        constexpr char *Green = (char *)"\033[32m";
        constexpr char *Yellow = (char *)"\033[33m";
        constexpr char *Blue = (char *)"\033[34m";
        constexpr char *Magenta = (char *)"\033[35m";
        constexpr char *Cyan = (char *)"\033[36m";
        constexpr char *White = (char *)"\033[37m";

        template <typename... TPrintables>
        std::ostream &Print(std::ostream &Output, TPrintables const &...) { return Output; }

        template <typename TPrintable, typename... TPrintables>
        std::ostream &Print(std::ostream &Output, TPrintable const &Printable, const TPrintables &...Printables)
        {
            Output << Printable;
            Print(Output, Printables...);
            return Output;
        }

       template <typename TTest>
        inline void MustThrow(TTest &&Test)
        try
        {
            Test();
            throw std::invalid_argument("");
        }
        catch (...)
        {
            return;
        }

        inline void Assert(bool Condition, std::string const &Message = "")
        {
            if (!Condition)
                throw std::invalid_argument(Message);
        }

        template <typename TTest>
        inline void Test(std::string const &Name, TTest &&Test)
        {
            try
            {
                Test();
                std::cout << Green << "Passed : " << Reset << Name << std::endl;
            }
            // catch (std::exception const &e)
            // {
            //     if (auto text = e.what())
            //         std::cout << Red << "Failed : " << Reset << Name << " With " << Blue << text << Reset << std::endl;
            //     else
            //         std::cout << Red << "Failed : " << Reset << Name << std::endl;
            // }
            catch (...)
            {
                std::cout << Red << "Failed : " << Reset << Name << std::endl;
            }
        }

        template <typename... TPrintables>
        std::ostream &Colored(std::ostream &Output, const char *Color, const TPrintables &...Printables)
        {
            Output << Color << "[" << DateTime::Now() << "]" << Reset << " : ";

            Print(Output, Printables...);

            std::cout.flush();

            return Output;
        }

        template <typename... TPrintables>
        inline std::ostream &Log(TPrintables const &...Printables)
        {
            Colored(std::cout, Green, Printables...);
            std::cout << std::endl;
            return std::cout;
        }

        template <typename... TPrintables>
        inline std::ostream &Warn(TPrintables const &...Printables)
        {
            Colored(std::cout, Yellow, Printables...);
            std::cout << std::endl;
            return std::cout;
        }

        template <typename... TPrintables>
        inline std::ostream &Error(TPrintables const &...Printables)
        {
            Colored(std::cerr, Red, Printables...);
            std::cout << std::endl;
            return std::cerr;
        }
    };
}