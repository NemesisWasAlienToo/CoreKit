#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <map>

#include <Machine.hpp>
#include <Format/Stringifier.hpp>
#include <Iterable/Queue.hpp>
#include <Network/Socket.hpp>
#include <Network/HTTP/Request.hpp>

namespace Core
{
    namespace Network
    {
        namespace HTTP
        {
            template <typename TMessage>
            struct Parser : Machine<void(Network::Socket)>
            {
                // @todo Change initial size

                Iterable::Queue<char> Queue{1024};

                size_t ContetLength = 0;
                size_t lenPos = 0;

                size_t bodyPos = 0;
                size_t bodyPosTmp = 0;

                TMessage Result;
                std::map<std::string, std::string>::iterator Iterator;

                void operator()()
                {
                    auto Client = Argument<0>();

                    {
                        Format::Stringifier Stream(Queue);

                        size_t Received = Client.Received();

                        // Read received data

                        char RequestBuffer[Received];

                        Client.Receive(RequestBuffer, Received);

                        Stream.Add(RequestBuffer, Received);
                    }

                    auto [Pointer, Size] = Queue.Chunk();

                    std::string_view Message{Pointer, Size};

                    CO_START;

                    // Take all the header

                    // @todo Optimize this

                    while (bodyPos == 0)
                    {
                        bodyPosTmp = Message.find("\r\n\r\n", bodyPosTmp);

                        if (bodyPosTmp == std::string::npos)
                        {
                            bodyPosTmp = Message.length() - 3;
                        }
                        else
                        {
                            bodyPos = bodyPosTmp + 4;
                            break;
                        }

                        CO_YIELD();
                    }

                    // Parse first line and headers

                    Result.ParseHeaders(Message, Result.ParseFirstLine(Message), bodyPos);

                    // Check for content length

                    Iterator = Result.Headers.find("Content-Length");

                    if (Iterator != Result.Headers.end() && !Iterator->second.empty())
                    {
                        // Get the length of content

                        try
                        {
                            ContetLength = std::stoul(Iterator->second);
                        }
                        catch (...)
                        {
                            throw HTTP::Response::BadRequest(Result.Version, {{"Connection", "close"}});
                        }

                        // Get the content

                        while (Message.length() - bodyPos < ContetLength)
                        {
                            CO_YIELD();
                        }

                        // fill the content

                        // Result.Content = std::string(Message.substr(bodyPos, ContetLength));
                        Result.Content = std::string{Message.substr(bodyPos, ContetLength)};

                        // @todo Maybe crop the used chunk of data and leave the rest to be parsed as a seperate request?

                        // Message = Message.substr(bodyPos + ContetLength);
                    }
                    else
                    {
                        // Read the transfer encoding

                        Iterator = Result.Headers.find("Transfer-Encoding");

                        if (Iterator == Result.Headers.end())
                        {
                            CO_TERMINATE();
                        }

                        // @todo Maybe parse weighted encoding?

                        if (Iterator->second == "chunked")
                        {
                            // Read the body chinks till the end

                            // @todo Implement later

                            throw HTTP::Response::NotImplemented(Result.Version, {{"Connection", "close"}});
                        }
                        else if (Iterator->second == "gzip")
                        {
                            // Read the body chinks till the end

                            // @todo Implement later

                            throw HTTP::Response::NotImplemented(Result.Version, {{"Connection", "close"}});
                        }
                        else
                        {
                            // Not implemented

                            throw HTTP::Response::NotImplemented(Result.Version, {{"Connection", "close"}});
                        }
                    }

                    CO_TERMINATE();

                    CO_END;
                }
            };
        }
    }
}