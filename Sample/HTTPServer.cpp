#include <iostream>
#include <string>

#include <Format/Stream.hpp>
#include <Network/HTTP/Server.hpp>
#include <Test.hpp>

using namespace Core::Network;

int main(int, char const *[])
{
    HTTP::Server Server({"0.0.0.0:8888"}, {5, 0}, 7);

    Test::Log("Server started");

    // Default route for the cases that no route matches the request

    Server.SetDefault(
        [](EndPoint const &, HTTP::Request const &Request)
        {
            return HTTP::Response::Text(Request.Version, HTTP::Status::NotFound, "This path does not exist");
        });

    // Simples route for home path

    Server.GET<"/">(
        [](EndPoint const &, HTTP::Request const &Request)
        {
            return HTTP::Response::Text(Request.Version, HTTP::Status::OK, "Hello world");
        });

    // More complex group route

    Server.GET<"/Static/[]", true>(
        [](EndPoint const &, HTTP::Request const &Request, std::string_view Folder, std::string_view File)
        {
            auto Params = Request.QueryParams();
            Request.Cookies(Params);
            Request.FormData(Params);

            auto BodyQueue = Iterable::Queue<char>(128);
            Format::Stream Sr(BodyQueue);

            Sr << "Hello world!\n" << "Accesing file : [" << Folder << '/' << File << ']';

            return HTTP::Response::Text(Request.Version, HTTP::Status::OK, Sr.ToString());
        });

    // // Building a simple filter that just forwards the request

    Server.FilterFrom(
        [](HTTP::Server::TFilter &&Next)
        {
            return [Next = std::move(Next)](Network::EndPoint const &Target, HTTP::Request &Request)
            {
                // Manipulate request before passing it down

                Request.Headers.insert_or_assign("Filter", "FirstFilter");

                // Pass the request down the chain

                auto Res = Next(Target, Request);

                // Manipulate response before returning it

                Res.SetContent(Res.Content + "\nAdded by filter");

                return Res;
            };
        });

    Server.Run();

    Server.GetInPool();

    // sleep(10);

    // Server.Stop();

    return 0;
}