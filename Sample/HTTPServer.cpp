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
    HTTP::Server Server({"0.0.0.0:8888"}, {5, 0}, 7);

    Test::Log("Server started");

    // Default route for the cases that no route matches the request

    Server.SetDefault(
        [](EndPoint const &, HTTP::Request const &Request, std::shared_ptr<void> &)
        {
            return HTTP::Response::HTML(Request.Version, HTTP::Status::NotFound, "<h1>404 Not Found</h1>");
        });

    // Simples route for home path

    Server.GET<"/">(
        [](EndPoint const &, HTTP::Request const &Request, std::shared_ptr<void> &)
        {
            return HTTP::Response::Text(Request.Version, HTTP::Status::OK, "Hello world");
        });

    // More complex group route

    Server.GET<"/Static/[]", true>(
        [](EndPoint const &, HTTP::Request const &Request, std::shared_ptr<void> &, std::string_view Folder, std::string_view File)
        {
            auto Params = Request.QueryParams();
            Request.Cookies(Params);
            Request.FormData(Params);

            auto BodyQueue = Iterable::Queue<char>(128);
            Format::Stream Sr(BodyQueue);

            Sr << "Hello world!\n"
               << "Accesing file : [" << Folder << '/' << File << ']';

            return HTTP::Response::Text(Request.Version, HTTP::Status::OK, Sr.ToString());
        });

    // Building a simple filter

    Server.FilterFrom(
        [](HTTP::Server::TFilter &&Next)
        {
            return [Next = std::move(Next)](Network::EndPoint const &Target, HTTP::Request &Request, std::shared_ptr<void> &Storage)
            {
                return Next(Target, Request, Storage);
            };
        });

    // A simple middleware

    Server.MiddlewareFrom(
        [](auto &&Next)
        {
            return [Next = std::move(Next)](Network::EndPoint const &Target, HTTP::Request &Request, std::shared_ptr<void> &Storage)
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

                return Next(Target, Request, Storage);
            };
        });

    Server.InitStorages(
              [](std::shared_ptr<void> &Storage)
              {
                  Storage = std::make_shared<std::string>("Storage data");
              })
        .SendFileThreshold(1024 * 100)
        .MaxHeaderSize(1024 * 1024 * 2)
        .MaxBodySize(1024 * 1024 * 10)
        .MaxConnectionCount(1024)
        .RequestBufferSize(512)
        .NoDelay(true)
        .HostName("Benchmark")
        .Run()
        .GetInPool();

    // sleep(10);

    // Server.Stop();

    return 0;
}