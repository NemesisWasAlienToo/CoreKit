#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <map>

#include <Machine.hpp>
#include <Format/Stringifier.hpp>
#include <Format/Serializer.hpp>
#include <Iterable/Queue.hpp>
#include <Network/Socket.hpp>
#include <Network/HTTP/Request.hpp>

// @todo Limit the number of clients

namespace Core
{
    namespace Network
    {
        namespace HTTP
        {
            struct Parser : Machine<void(Network::Socket const *)>
            {
                static constexpr auto HeaderLimit = 16 * 1024;
                static constexpr auto ContentLimit = 16 * 1024;

                // @todo Change initial size

                Iterable::Queue<char> Queue{1024};

                size_t ContetLength = 0;
                size_t lenPos = 0;

                size_t bodyPos = 0;
                size_t bodyPosTmp = 0;

                HTTP::Request Result;
                std::unordered_map<std::string, std::string>::iterator Iterator;

                // @todo Make this asynchronus

                void Reset()
                {
                    // Crop buffer's content

                    Machine::Reset();
                    Queue.Free(bodyPos + ContetLength);

                    if (Queue.Length() > 0)
                        Queue.Resize(Queue.Capacity());

                    ContetLength = 0;
                    lenPos = 0;

                    bodyPos = 0;
                    bodyPosTmp = 0;

                    Result = HTTP::Request();
                    Iterator = Result.Headers.end();
                }

                void Clear()
                {
                    Machine::Reset();
                    Queue.Free();

                    ContetLength = 0;
                    lenPos = 0;

                    bodyPos = 0;
                    bodyPosTmp = 0;

                    Result = HTTP::Request();
                    Iterator = Result.Headers.end();
                }

                void Continue100()
                {
                    const auto &Client = *Argument<0>();

                    auto ExpectIterator = Result.Headers.find("Expect");

                    if (ExpectIterator != Result.Headers.end() && ExpectIterator->second == "100-continue")
                    {
                        // Ensure version is HTTP 1.1

                        if (Result.Version[5] == '1' && Result.Version[7] == '0')
                        {
                            throw HTTP::Response::From(Result.Version, HTTP::Status::ExpectationFailed, {{"Connection", "close"}});
                        }

                        if (ContetLength == 0)
                        {
                            throw HTTP::Response::From(Result.Version, HTTP::Status::BadRequest, {{"Connection", "close"}});
                        }

                        // Respond to 100-continue

                        // @todo Maybe hanlde HTTP 2.0 later too?

                        constexpr auto ContinueResponse = "HTTP/1.1 100 Continue\r\n\r\n";

                        Iterable::Queue<char> Temp(sizeof(ContinueResponse), false);
                        Format::Stringifier ContinueStream(Temp);

                        Temp.Add(ContinueResponse, sizeof(ContinueResponse));

                        // Send the response

                        while (Temp.Length() > 0)
                        {
                            Client << ContinueStream;
                        }
                    }
                }

                void operator()()
                {
                    const auto &Client = *Argument<0>();

                    {
                        Format::Stringifier Stream(Queue);

                        size_t Received = Client.Received();

                        if (Received == 0)
                            Terminate();

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
                    // @todo Set limit for header size

                    // while (bodyPos == 0 && Queue.Length() < HeaderLimit)
                    // Queue.Length() < HeaderLimit -> BadRequest

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

                    {
                        size_t TempIndex = 0;

                        try
                        {
                            TempIndex = Result.ParseFirstLine(Message);
                        }
                        catch (...)
                        {
                            throw HTTP::Response::From(Result.Version, HTTP::Status::BadRequest, {{"Connection", "close"}});
                        }

                        // Check for version

                        if (Result.Version[0] != '1' || (Result.Version[2] != '0' && Result.Version[2] != '1'))
                        {
                            throw HTTP::Response::From(Result.Version, HTTP::Status::HTTPVersionNotSupported, {{"Connection", "close"}});
                        }

                        Result.ParseHeaders(Message, TempIndex, bodyPos);
                    }

                    // Check for content length

                    Iterator = Result.Headers.find("Content-Length");

                    if (Iterator != Result.Headers.end() && !Iterator->second.empty())
                    {
                        // Get the length of content

                        try
                        {
                            ContetLength = std::stoull(Iterator->second);
                        }
                        catch (...)
                        {
                            throw HTTP::Response::From(Result.Version, HTTP::Status::BadRequest, {{"Connection", "close"}});
                        }

                        // Check if the length is in valid range

                        if (ContetLength > ContentLimit)
                        {
                            throw HTTP::Response::From(Result.Version, HTTP::Status::RequestEntityTooLarge, {{"Connection", "close"}});
                        }

                        // Handle 100-continue

                        Continue100();

                        // Get the content

                        while (Message.length() - bodyPos < ContetLength)
                        {
                            CO_YIELD();
                        }

                        // fill the content

                        Result.Content = Message.substr(bodyPos, ContetLength);

                        // @todo Maybe crop the used chunk of data and leave the rest to be parsed as a seperate request?

                        // Message = Message.substr(bodyPos + ContetLength);
                    }
                    else
                    {
                        // Read the transfer encoding

                        Iterator = Result.Headers.find("Transfer-Encoding");

                        if (Iterator == Result.Headers.end())
                        {
                            Continue100();

                            CO_TERMINATE();
                        }

                        // @todo Maybe parse weighted encoding too?

                        else if (Iterator->second == "chunked")
                        {
                            // Read the body chunks untill the end

                            // @todo Implement later
                            // @todo Check content length to be in the acceptable range
                            // throw HTTP::Response::RequestEntityTooLarge(Result.Version, {{"Connection", "close"}});

                            throw HTTP::Response::From(Result.Version, HTTP::Status::NotImplemented, {{"Connection", "close"}});
                        }
                        else if (Iterator->second == "gzip")
                        {
                            // Read the body chinks till the end

                            // @todo Implement later
                            // @todo Check content length to be in the acceptable range
                            // throw HTTP::Response::RequestEntityTooLarge(Result.Version, {{"Connection", "close"}});

                            // @todo Implement later

                            throw HTTP::Response::From(Result.Version, HTTP::Status::NotImplemented, {{"Connection", "close"}});
                        }
                        else
                        {
                            // Not implemented

                            throw HTTP::Response::From(Result.Version, HTTP::Status::NotImplemented, {{"Connection", "close"}});
                        }
                    }

                    CO_TERMINATE();

                    CO_END;
                }
            };
        }
    }
}