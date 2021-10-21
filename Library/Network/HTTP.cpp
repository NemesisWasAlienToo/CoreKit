#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <map>

#include "Iterable/List.cpp"

namespace Core
{
    namespace Network
    {
        namespace HTTP
        {
            class Common
            {
            protected:
                Common() = default;
                ~Common() {}

                Iterable::List<std::string> _SplitString(const std::string &Text, const std::string &Seprator)
                {
                    Iterable::List<std::string> Temp;
                    size_t start = 0;
                    size_t end;

                    std::string Sub;

                    while (true)
                    {
                        if ((end = Text.find(Seprator, start)) == std::string::npos)
                        {
                            if (!(Sub = Text.substr(start)).empty())
                            {
                                Temp.Add(Sub);
                            }

                            break;
                        }

                        Sub = Text.substr(start, end - start);
                        Temp.Add(Sub);
                        start = end + 1;
                    }

                    return Temp;
                }

            public:
                std::string Version;
                std::map<std::string, std::string> Headers;
                size_t Length;
                std::string Body;

                virtual std::string ToString() const { return ""; };

                virtual void From(std::string Text){}
            };

            class Request : public Common
            {
            public:
                std::string Type;
                std::string Path;

                Request() = default;
                ~Request() {}

                std::string ToString() const
                {
                    std::string str = Type + " " + Path + " HTTP/" + Version + "\r\n";
                    for (auto const &kv : Headers)
                        str += kv.first + ": " + kv.second + "\r\n";

                    return str + "\r\n" + Body;
                }

                virtual void From(std::string Text)
                {
                    //
                }
            };

            class Response : public Common
            {
            public:
                unsigned short Status;
                std::string Brief;

                std::string ToString() const
                {
                    std::string str = "HTTP/" + Version + std::to_string(Status) + Brief + "\r\n";
                    for (auto const &kv : Headers)
                        str += kv.first + ": " + kv.second + "\r\n";

                    return str + "\r\n" + Body;
                }

                virtual void From(std::string Text)
                {
                    //
                }
            };

            Response Send(const Request& request){
                
            }
        }
    }
}