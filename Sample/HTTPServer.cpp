#include <iostream>
#include <string>
#include <memory>
#include <set>

#include <Format/Stream.hpp>
#include <Network/HTTP/Server.hpp>
#include <File.hpp>
#include <Test.hpp>

using namespace Core::Network;

int main(int, char const *[])
{
    // Create an instance of http runner with a 2 working threads

    HTTP::Server Server(2);

    Test::Log("Server started");

    // Default route for the cases that no route matches the request

    // Optional fallback router

    Server.SetDefault(
        [](HTTP::Connection::Context &, HTTP::Request &Request)
        {
            return HTTP::Response::HTML(Request.Version, HTTP::Status::NotFound, "<h1>404 Not Found</h1>");
        });

    // Global Filter will be called if no route is matched and before Default is called
    // This is an extremely simple static file filter

    Server.Filter(
        [SendFileThreshold = static_cast<size_t>(1024 * 100)](HTTP::Connection::Context &Context, HTTP::Request &Request, auto &&Next) -> std::optional<HTTP::Response>
        {
            auto FileName = Request.Path.substr(1);

            if (File::IsRegular(FileName))
            {
                auto StaticFile = File::Open(FileName, File::Binary | File::ReadOnly);

                if (!Context.CanUseSendFile() || StaticFile.Size() <= SendFileThreshold)
                    return Context.SendResponse(
                        HTTP::Response::Type(Request.Version, HTTP::Status::OK, HTTP::GetContentType(File::GetExtension(FileName)), StaticFile.ReadAll()));

                return Context.SendResponse(
                    HTTP::Response::Type(Request.Version, HTTP::Status::OK, HTTP::GetContentType(File::GetExtension(FileName))),
                    std::move(StaticFile));
            }

            return Next(Context, Request);
        });

    // Global middleware

    Server.Middleware(
        [](HTTP::Connection::Context &Context, Network::HTTP::Request &Request, auto &&Next)
        {
            return Next(Context, Request);
        });

    // Simple route

    Server.GET<"/">(
        [](HTTP::Connection::Context &, HTTP::Request &Request)
        {
            return HTTP::Response::HTML(Request.Version, HTTP::Status::OK, "<h1>Hello world</h1>");
        });

    // Route with one argument

    Server.GET<"/Static/[]">(
        [](HTTP::Connection::Context &, HTTP::Request &Request, std::string_view &&Param)
        {
            return HTTP::Response::HTML(Request.Version, HTTP::Status::OK, std::string{Param});
        });

    // Async route which delays response for 2 seconds

    Server.GET<"/Delayed">(
        [](HTTP::Connection::Context &Context, HTTP::Request &Request) -> std::optional<HTTP::Response>
        {
            Context.Schedule(
                {2, 0},
                [Context, Version = Request.Version]
                {
                    Context.SendResponse(HTTP::Response::HTML(Version, HTTP::Status::OK, "<h1>Hello world, but delayed!</h1>"));
                });

            return std::nullopt;
        });

    // Route which watches for connections to disconnect after visiting this route

    Server.GET<"/Notify">(
        [&](HTTP::Connection::Context &Context, HTTP::Request &Request) -> std::optional<HTTP::Response>
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

            return HTTP::Response::HTML(Request.Version, HTTP::Status::OK, "<h1>Hello world, but notified!</h1>");
        });

    // Route which will switch to some other protocol after sending a response

    Server.GET<"/Upgrade">(
        [](HTTP::Connection::Context &Context, HTTP::Request &Request) -> std::optional<HTTP::Response>
        {
            Context.ListenFor(ePoll::Out);

            Context.OnSent(
                [Context]() mutable
                {
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

                                std::cout << "Got data\n";

                                Iterable::Span<char> Data = Context.FileAs<Network::Socket>().Receive();
                            }
                        },
                        {0, 0});
                });

            return HTTP::Response::HTML(Request.Version, HTTP::Status::OK, "<h1>Hello world, upgraded!</h1>");
        });

    // Simple route which reports from which listening endpoint it was originated

    Server.GET<"/Source">(
        [](HTTP::Connection::Context &Context, HTTP::Request &Request)
        {
            return HTTP::Response::HTML(Request.Version, HTTP::Status::OK, Context.Source.ToString() + (Context.IsSecure() ? " Is secure" : " Isn't secure"));
        });

    Server

        // Init thread storages and other settings

        .InitStorages(
            [](std::shared_ptr<void> &Storage)
            {
                Storage = std::make_shared<std::string>("Storage data");
            })

        //

        .OnError(
            [](HTTP::Connection::Context &Context, Network::HTTP::Response &)
            {
                std::cout << "Error on " << Context.Target << std::endl;
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

        .MaxFileSize(1024 * 1024 * 10)
        .MaxHeaderSize(1024 * 1024 * 2)
        .MaxBodySize(1024 * 1024 * 10)
        .MaxConnectionCount(1024)
        .RequestBufferSize(256)
        .ResponseBufferSize(256)

        // Enables TCP nodelay

        .NoDelay(true)

        // Host name in the response header

        .HostName("Benchmark")

        // Starts the thread pool

        .Run()

        // Joins this thread to thread pool

        .GetInPool();

    return 0;
}