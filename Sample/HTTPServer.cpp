#include <iostream>
#include <string>
#include <memory>

#include <Format/Stream.hpp>
#include <Network/HTTP/Server.hpp>
#include <File.hpp>
#include <Test.hpp>

using namespace Core::Network;

int main(int, char const *[])
{
    HTTP::Server Server({"0.0.0.0:8888"}, {5, 0}, 2);

    Test::Log("Server started");

    // Default route for the cases that no route matches the request

    Server.SetDefault(
        [](HTTP::ConnectionContext &, HTTP::Request &Request)
        {
            return HTTP::Response::HTML(Request.Version, HTTP::Status::NotFound, "<h1>404 Not Found</h1>");
        });

    Server.Filter(
        [](HTTP::ConnectionContext &Context, HTTP::Request &Request, auto &&Next) -> std::optional<HTTP::Response>
        {
            auto FileName = Request.Path.substr(1);

            try
            {
                if (File::IsRegular(FileName))
                {
                    return HTTP::Response::FromPath(Request.Version, HTTP::Status::OK, FileName);
                }
            }
            catch (std::system_error const &e)
            {
                std::cout << FileName << " does not exist" << std::endl;
            }

            return Next(Context, Request);
        });

    Server.GET<"/">(
        [](HTTP::ConnectionContext &, HTTP::Request &Request)
        {
            return HTTP::Response::HTML(Request.Version, HTTP::Status::OK, "<h1>Hello world</h1>");
        });

    Server.GET<"/Static/[]">(
        [](HTTP::ConnectionContext &, HTTP::Request &Request, std::string_view &&Param)
        {
            return HTTP::Response::HTML(Request.Version, HTTP::Status::OK, std::string{Param});
        });

    Server.Middleware(
        [](HTTP::ConnectionContext &Context, Network::HTTP::Request &Request, auto &&Next)
        {
            return Next(Context, Request);
        });  

    Server.InitStorages(
              [](std::shared_ptr<void> &Storage)
              {
                  Storage = std::make_shared<std::string>("Storage data");
              })
        .OnError(
            [](ConnectionContext & Context, Network::HTTP::Response &)
            {
                std::cout << "Error on " << Context.Target << std::endl;
            })
        .SendFileThreshold(1024 * 100)
        .MaxHeaderSize(1024 * 1024 * 2)
        .MaxBodySize(1024 * 1024 * 10)
        .MaxConnectionCount(1024)
        .RequestBufferSize(256)
        .ResponseBufferSize(256)
        .NoDelay(true)
        .HostName("Benchmark")
        .Run()
        .GetInPool();

    return 0;
}