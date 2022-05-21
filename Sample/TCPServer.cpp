#include <iostream>
#include <Network/TCPServer.hpp>

using namespace Core;

auto BuildClientHandler(Async::EventLoop &Loop)
{
    return [&](Async::EventLoop *This, ePoll::Entry &Item, Async::EventLoop::Entry &Self)
    {
        if (Item.Happened(ePoll::In))
        {
            // Read data

            Network::Socket &Server = *static_cast<Network::Socket *>(&Self.File);

            if (!Server.Received())
            {
                Loop.Remove(Self.Iterator);
                return;
            }

            if (Self.Parser.IsStarted())
            {
                Self.Parser.Continue();
            }
            else
            {
                Self.Parser.Start(&Server);
            }

            if (Self.Parser.IsFinished())
            {
                // Process request

                Network::HTTP::Request &Request = Self.Parser.Result;

                // Append response to buffer

                {
                    auto Response = Network::HTTP::Response::Text(Request.Version, Network::HTTP::Status::OK, "Hello, World!");

                    // Response.Headers.insert({"Connection", "close"});
                    // Response.Headers.insert({"Connection", "keep-alive"});

                    Response.AppendToBuffer(Self.Buffer);
                }

                // Modify events

                Loop.Modify(Self, ePoll::In | ePoll::Out);

                // Reset Parser

                Self.Parser.Reset();
            }
        }

        if (Item.Happened(ePoll::Out))
        {
            // Check if has data to send

            if (Self.Buffer.IsEmpty())
            {
                // If not stop listenning to out event

                Self.Buffer.Free();

                Loop.Modify(Self, ePoll::In);

                return;
            }

            // Write data

            Format::Stringifier Ser(Self.Buffer);

            Self.File << Ser;
        }

        if (Item.Happened(ePoll::HangUp) || Item.Happened(ePoll::Error))
        {
            Loop.Remove(Self.Iterator);
        }
    };
}

int main(int argc, char const *argv[])
{
    constexpr size_t ThreadCount = 1;

    Network::TCPServer Server({"0.0.0.0:8888"}, BuildClientHandler, ThreadCount);

    Server.Run();

    while (true)
    {
        std::string input;

        std::cin >> input;

        if (input == "exit")
            break;
    }

    Server.Stop();

    return 0;
}