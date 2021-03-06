#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <map>

#include <Machine.hpp>
#include <Format/Stream.hpp>
#include <Iterable/Queue.hpp>
#include <Network/Socket.hpp>
#include <Network/HTTP/Request.hpp>

namespace Core::Network::HTTP
{
    struct Parser : Machine<void(Network::Socket const &)>
    {
        size_t HeaderLimit = 16 * 1024;
        size_t ContentLimit = 8 * 1024 * 1024;
        size_t RequestBufferSize = 1024;

        Parser() = default;
        Parser(size_t headerLimit, size_t contentLimit, size_t SendBufferSize) : Machine(), HeaderLimit(headerLimit), ContentLimit(contentLimit), RequestBufferSize(SendBufferSize), Queue(RequestBufferSize) {}

        Iterable::Queue<char> Queue;

        size_t ContetLength = 0;
        size_t lenPos = 0;

        size_t bodyPos = 0;
        size_t bodyPosTmp = 0;

        HTTP::Request Result;
        std::unordered_map<std::string, std::string>::iterator Iterator;

        void Reset()
        {
            // Crop buffer's content

            Machine::Reset();
            Queue.Free(bodyPos + ContetLength);

            // @todo Optimize Resize

            if (Queue.Length())
                Queue.Resize(Queue.Capacity());

            ContetLength = 0;
            lenPos = 0;

            bodyPos = 0;
            bodyPosTmp = 0;

            // Clean request

            // Result = HTTP::Request();

            {
                Result.Headers.clear();
                Queue.Resize(1024);
            }

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

        // @todo Make this asynchronus

        void Continue100(Network::Socket const &Client)
        {
            auto ExpectIterator = Result.Headers.find("Expect");

            if (ExpectIterator != Result.Headers.end() && ExpectIterator->second == "100-continue")
            {
                // Ensure version is HTTP 1.1

                if (Result.Version[5] == '1' && Result.Version[7] == '0')
                {
                    throw HTTP::Response::From(Result.Version, HTTP::Status::ExpectationFailed, {{"Connection", "close"}}, "");
                }

                if (ContetLength == 0)
                {
                    throw HTTP::Response::From(Result.Version, HTTP::Status::BadRequest, {{"Connection", "close"}}, "");
                }

                // Respond to 100-continue

                // @todo Maybe hanlde HTTP 2.0 later too?

                constexpr auto ContinueResponse = "HTTP/1.1 100 Continue\r\n\r\n";

                Iterable::Queue<char> Temp(sizeof(ContinueResponse), false);
                Format::Stream ContinueStream(Temp);

                Temp.CopyFrom(ContinueResponse, sizeof(ContinueResponse));

                // Send the response

                while (Temp.Length() > 0)
                {
                    Client << ContinueStream;
                }
            }
        }

        void operator()(Network::Socket const &Client) override
        {
            {
                Format::Stream Stream(Queue);

                Client >> Stream;
            }

            auto [Pointer, Size] = Queue.DataChunk();

            std::string_view Message{Pointer, Size};

            CO_START;

            // Take all the header

            while (bodyPos == 0)
            {
                bodyPosTmp = Message.find("\r\n\r\n", bodyPosTmp);

                if (bodyPosTmp == std::string::npos)
                {
                    // @todo Optimize this
                    // Check header size

                    if (Message.length() > HeaderLimit)
                    {
                        throw HTTP::Response::From(Result.Version, HTTP::Status::RequestEntityTooLarge, {{"Connection", "close"}}, "");
                    }

                    bodyPosTmp = Message.length() - 3;
                }
                else
                {
                    bodyPos = bodyPosTmp + 4;

                    if (bodyPos > HeaderLimit)
                    {
                        throw HTTP::Response::From(Result.Version, HTTP::Status::RequestEntityTooLarge, {{"Connection", "close"}}, "");
                    }

                    break;
                }

                CO_YIELD();
            }

            // Parse first line and headers

            {
                size_t TempIndex = 0;

                // @todo Remove try catch

                try
                {
                    TempIndex = Result.ParseFirstLine(Message);
                }
                catch (...)
                {
                    throw HTTP::Response::From(Result.Version, HTTP::Status::BadRequest, {{"Connection", "close"}}, "");
                }

                // Check for version

                if (Result.Version[0] != '1' || (Result.Version[2] != '0' && Result.Version[2] != '1'))
                {
                    throw HTTP::Response::From(Result.Version, HTTP::Status::HTTPVersionNotSupported, {{"Connection", "close"}}, "");
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
                    throw HTTP::Response::From(Result.Version, HTTP::Status::BadRequest, {{"Connection", "close"}}, "");
                }

                // Check if the length is in valid range

                if (ContetLength > ContentLimit)
                {
                    throw HTTP::Response::From(Result.Version, HTTP::Status::RequestEntityTooLarge, {{"Connection", "close"}}, "");
                }

                // Handle 100-continue

                Continue100(Client);

                // Get the content

                while (Message.length() - bodyPos < ContetLength)
                {
                    CO_YIELD();
                }

                // fill the content

                Result.Content = Message.substr(bodyPos, ContetLength);
            }
            else
            {
                // Read the transfer encoding

                Iterator = Result.Headers.find("Transfer-Encoding");

                if (Iterator == Result.Headers.end())
                {
                    Continue100(Client);

                    CO_TERMINATE();
                }

                // @todo Maybe parse weighted encoding too?

                else if (Iterator->second == "chunked")
                {
                    // Read the body chunks untill the end

                    // @todo Implement later
                    // @todo Check content length to be in the acceptable range
                    // throw HTTP::Response::RequestEntityTooLarge(Result.Version, {{"Connection", "close"}});

                    throw HTTP::Response::From(Result.Version, HTTP::Status::NotImplemented, {{"Connection", "close"}}, "");
                }
                else if (Iterator->second == "gzip")
                {
                    // Read the body chinks till the end

                    // @todo Implement later
                    // @todo Check content length to be in the acceptable range
                    // throw HTTP::Response::RequestEntityTooLarge(Result.Version, {{"Connection", "close"}});

                    // @todo Implement later

                    throw HTTP::Response::From(Result.Version, HTTP::Status::NotImplemented, {{"Connection", "close"}}, "");
                }
                else
                {
                    // Not implemented

                    throw HTTP::Response::From(Result.Version, HTTP::Status::NotImplemented, {{"Connection", "close"}}, "");
                }
            }

            CO_TERMINATE();

            CO_END;
        }
    };
}