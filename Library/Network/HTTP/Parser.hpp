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
    struct Parser : Machine<void()>
    {
        size_t HeaderLimit = 16 * 1024;
        size_t ContentLimit = 8 * 1024 * 1024;
        size_t RequestBufferSize = 1024;

        Parser(size_t headerLimit, size_t contentLimit, size_t SendBufferSize, Iterable::Queue<char> &queue) : Machine(), HeaderLimit(headerLimit), ContentLimit(contentLimit), RequestBufferSize(SendBufferSize), Queue(queue)
        {
            Queue = Iterable::Queue<char>(SendBufferSize);
        }

        Iterable::Queue<char> &Queue;

        size_t ContentLength = 0;
        size_t lenPos = 0;

        size_t bodyPos = 0;
        size_t bodyPosTmp = 0;

        HTTP::Request Result;
        std::unordered_map<std::string, std::string>::iterator Iterator;

        bool RequiresContinue100 = false;

        void Reset()
        {
            // Crop buffer's content

            Machine::Reset();
            Queue.Free(bodyPos + ContentLength);

            // @todo Optimize Resize

            if (Queue.Length())
                Queue.Resize(Queue.Capacity());

            ContentLength = 0;
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
            RequiresContinue100 = false;
        }

        // @todo Make this asynchronous

        void Continue100()
        {
            auto ExpectIterator = Result.Headers.find("Expect");

            if (ExpectIterator != Result.Headers.end() && ExpectIterator->second == "100-continue")
            {
                RequiresContinue100 = true;
            }
        }

        bool HasBody()
        {
            return bool(bodyPos);
        }

        void operator()() override
        {
            auto [Pointer, Size] = Queue.DataChunk();

            std::string_view Message{Pointer, Size};

            CO_START;

            // Take all the header

            while (bodyPos == 0)
            {
                if (HeaderLimit && Message.length() > HeaderLimit)
                {
                    throw HTTP::Status::RequestEntityTooLarge;
                }

                bodyPosTmp = Message.find("\r\n\r\n", bodyPosTmp);

                if (bodyPosTmp != std::string::npos)
                {
                    bodyPos = bodyPosTmp + 4;

                    break;
                }

                bodyPosTmp = Message.length() - 3;

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
                    throw HTTP::Status::BadRequest;
                }

                // Check for version

                if (Result.Version[0] != '1' || (Result.Version[2] != '0' && Result.Version[2] != '1'))
                {
                    throw HTTP::Status::HTTPVersionNotSupported;
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
                    ContentLength = std::stoull(Iterator->second);
                }
                catch (...)
                {
                    throw HTTP::Status::BadRequest;
                }

                // Check if the length is in valid range

                if (ContentLimit && ContentLength > ContentLimit)
                {
                    throw HTTP::Status::RequestEntityTooLarge;
                }

                // Handle 100-continue

                Continue100();

                // Get the content

                while (Message.length() - bodyPos < ContentLength)
                {
                    CO_YIELD();
                }

                // fill the content

                Result.Content = Message.substr(bodyPos, ContentLength);
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
                    // @todo Implement later
                    // @todo Check content length to be in the acceptable range

                    throw HTTP::Status::NotImplemented;
                }
                else if (Iterator->second == "gzip")
                {
                    // Read the body chinks till the end

                    // @todo Implement later
                    // @todo Check content length to be in the acceptable range

                    throw HTTP::Status::NotImplemented;
                }
                else
                {
                    // Not implemented

                    throw HTTP::Status::NotImplemented;
                }
            }

            CO_TERMINATE();

            CO_END;
        }
    };
}