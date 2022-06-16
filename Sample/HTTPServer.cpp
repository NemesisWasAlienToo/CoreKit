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
        [](EndPoint const &, HTTP::Request const &Request, std::shared_ptr<void>)
        {
            return HTTP::Response::Text(Request.Version, HTTP::Status::NotFound, "This path does not exist");
        });

    // Simples route for home path

    Server.GET<"/">(
        [](EndPoint const &, HTTP::Request const &Request, std::shared_ptr<void>)
        {
            return HTTP::Response::Text(Request.Version, HTTP::Status::OK, "Hello world");
        });

    // More complex group route

    Server.GET<"/Static/[]", true>(
        [](EndPoint const &, HTTP::Request const &Request, std::shared_ptr<void>, std::string_view Folder, std::string_view File)
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
            return [Next = std::move(Next)](Network::EndPoint const &Target, HTTP::Request &Request, std::shared_ptr<void> Storage)
            {
                if (Request.Path == "/index.html")
                {
                    std::shared_ptr<File> Index = std::make_shared<File>(File::Open("html/index.html", File::ReadOnly));

                    return HTTP::Response::HTML(Request.Version, HTTP::Status::OK, Index);
                }

                return Next(Target, Request, Storage);
            };
        });

    // How to pass data down to the next filter

    Server.FilterFrom(
        [](HTTP::Server::TFilter &&Next)
        {
            return [Next = std::move(Next)](Network::EndPoint const &Target, HTTP::Request &Request, std::shared_ptr<void> Storage)
            {
                if (!Storage)
                    Storage = std::make_shared<std::string>("Data from top filter");

                return Next(Target, Request, Storage);
            };
        });

    Server.Run();

    Server.GetInPool();

    // sleep(10);

    // Server.Stop();

    return 0;
}