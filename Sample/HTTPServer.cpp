#include <iostream>
#include <string>
#include <memory>
#include <set>
#include <fmt/core.h>

#include <Network/HTTP/Modules/Router.hpp>
#include <Network/HTTP/Server.hpp>
#include <Format/Stream.hpp>
#include <File.hpp>
#include <Test.hpp>
#include <Network/DNS.hpp>

using namespace Core::Network;

// A simple printer module which just adds a print function

template <typename T>
struct Printer
{
    T &Print(std::string const &Message)
    {
        fmt::print("{}\n", Message);
        return static_cast<T &>(*this);
    }
};

int main(int, char const *[])
{
    // Create an instance of http runner with a 2 working threads

    HTTP::Server<HTTP::Modules::Router, Printer> Server(2);

    // Default route for the cases that no route matches the request

    // Optional fallback router

    Server.SetDefault(
              [](HTTP::Connection::Context &Context, HTTP::Request &Request)
              {
                  Context.SendResponse(HTTP::Response::HTML(Request.Version, HTTP::Status::NotFound, "<h1>404 Not Found</h1>"));
              })

        // Global Filter will be called if no route is matched and before Default is called
        // This is an extremely simple static file filter

        .Filter(
            [SendFileThreshold = static_cast<size_t>(1024 * 100)](HTTP::Connection::Context &Context, HTTP::Request &Request, auto &&Next)
            {
                auto FileName = Request.Path.substr(1);

                if (File::IsRegular(FileName))
                {
                    auto StaticFile = File::Open(FileName, File::Binary | File::ReadOnly);

                    if (!Context.CanUseSendFile() || StaticFile.Size() <= SendFileThreshold)
                        Context.SendResponse(
                            HTTP::Response::Type(Request.Version, HTTP::Status::OK, HTTP::GetContentType(File::GetExtension(FileName)), StaticFile.ReadAll()));

                    Context.SendResponse(
                        HTTP::Response::Type(Request.Version, HTTP::Status::OK, HTTP::GetContentType(File::GetExtension(FileName))),
                        std::move(StaticFile));
                }

                Next(Context, Request);
            })

        // Global middleware

        .Middleware(
            [](HTTP::Connection::Context &Context, Network::HTTP::Request &Request, auto &&Next)
            {
                Next(Context, Request);
            })

        // Global Processor post-processes the response

        .Processor(
            [](HTTP::Connection::Context &Context, Network::HTTP::Response &&Response, File &&file, size_t Length, auto &&Next)
            {
                Next(Context, std::move(Response), std::move(file), Length);
            })

        // Simple route

        .GET<"/">(
            [](HTTP::Connection::Context &Context, HTTP::Request &Request)
            {
                Context.SendResponse(HTTP::Response::HTML(Request.Version, HTTP::Status::OK, "<h1>Hello world</h1>"));
            })

        // Route with one argument and grouped route

        .GET<"/Static/[]/*">(
            [](HTTP::Connection::Context &Context, HTTP::Request &Request, std::string_view Param, std::string_view Rest)
            {
                Context.SendResponse(
                    HTTP::Response::HTML(Request.Version, HTTP::Status::OK, std::string{Param} + " : " + std::string{Rest}));
            })

        // Async route which delays response for 2 seconds

        .GET<"/Delayed">(
            [](HTTP::Connection::Context &Context, HTTP::Request &Request)
            {
                Context.Schedule(
                    {2, 0},
                    [Context, Version = Request.Version]() mutable
                    {
                        Context.SendResponse(HTTP::Response::HTML(Version, HTTP::Status::OK, "<h1>Hello world, but delayed!</h1>"));
                    });
            })

        // Route which watches for connections to disconnect after visiting this route

        .GET<"/Notify">(
            [&](HTTP::Connection::Context &Context, HTTP::Request &Request)
            {
                static std::set<EndPoint> Peers;
                static std::mutex Lock;

                {
                    std::scoped_lock l(Lock);

                    if (!Peers.contains(Context.Target))
                    {
                        Peers.emplace(Context.Target);

                        Context.OnRemove(
                            [Context]
                            {
                                std::cout << Context.Target << " disconnected\n";

                                {
                                    std::scoped_lock l(Lock);

                                    Peers.erase(Context.Target);
                                }
                            });
                    }
                }

                Context.SendResponse(HTTP::Response::HTML(Request.Version, HTTP::Status::OK, "<h1>Hello world, but notified!</h1>"));
            })

        // Route which will switch to some other protocol after sending a response

        .GET<"/Upgrade">(
            [](HTTP::Connection::Context &Context, HTTP::Request &Request)
            {
                Context.ListenFor(ePoll::Out);

                Context.OnSent(
                    [Context]() mutable
                    {
                        if (!Context.WillClose())
                            Context.Upgrade(
                                [](Async::EventLoop::Context &Context, ePoll::Entry &Entry) mutable
                                {
                                    if (Entry.Happened(ePoll::In))
                                    {
                                        if (!Context.FileAs<Network::Socket>().Received())
                                        {
                                            Context.Remove();
                                            return;
                                        }

                                        Iterable::Span<char> Data = Context.FileAs<Network::Socket>().Receive();
                                    }
                                },
                                {0, 0});
                    });

                Context.SendResponse(HTTP::Response::HTML(Request.Version, HTTP::Status::OK, "<h1>Hello world, upgraded!</h1>"));
            })

        // Simple route which reports from which listening endpoint it was originated

        .GET<"/Source">(
            [](HTTP::Connection::Context &Context, HTTP::Request &Request)
            {
                Context.SendResponse(HTTP::Response::HTML(Request.Version, HTTP::Status::OK, Context.Source.ToString() + (Context.IsSecure() ? " Is secure" : " Isn't secure")));
            })

        // Init thread storages and other settings

        .InitStorages(
            [](std::shared_ptr<void> &Storage)
            {
                Storage = std::make_shared<std::string>("Storage data");
            })

        // Ignore SIGPIPE

        .IgnoreBrokenPipe()

        // Idle time out for connections

        .Timeout({5, 0})

        // It's possible to have multiple listeners

        // HTTP Listener

        .Listen({"0.0.0.0:8888"})

        // HTTPS Listener

        .Listen({"0.0.0.0:4444"}, "Cert.pem", "Key.pem")

        // Size configurations
        // Zero means no limit

        .MaxHeaderSize(1024 * 1024 * 2)
        .MaxBodySize(1024 * 1024 * 10)
        .MaxConnections(1024)
        .RequestBufferSize(256)
        .ResponseBufferSize(256)

        // Enables TCP nodelay

        .NoDelay(true)

        // Starts the thread pool

        .Run()

        // Print a prompt

        .Print("Server started")

        // Joins this thread to thread pool

        .GetInPool();

    return 0;
}