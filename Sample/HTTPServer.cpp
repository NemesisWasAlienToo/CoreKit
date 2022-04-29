#include <iostream>
#include <string>
#include <tuple>
#include <functional>

#include <Network/HTTP/Server.hpp>

using namespace Core::Network;

int main(int argc, char const *argv[])
{
    HTTP::Server Server({"0.0.0.0:8888"}, {5, 0});

    Server.SetDefault(
        "/",
        [](const EndPoint &, const HTTP::Request &Request, std::map<std::string, std::string> &Parameters)
        {
            Request.Cookies(Parameters);

            for (auto &Pair : Parameters)
            {
                std::cout << Pair.first << ": " << Pair.second << std::endl;
            }

            return HTTP::Response::Text(HTTP::Status::NotFound, "This path does not exist")
                .SetCookie("Name", "TestName")
                .SetCookie("Family", "TestFamily")
                .SetCookie("Id", "TestFamily");
        });

    Server.GET(
        "/Home",
        [](const EndPoint &, const HTTP::Request &Request, std::map<std::string, std::string> &Parameters)
        {
            return HTTP::Response::Redirect("/Home/1");
        });

    Server.POST(
        "/Home/[Index]",
        [](const EndPoint &, const HTTP::Request &Request, std::map<std::string, std::string> &Parameters)
        {
            Request.FormData(Parameters);

            // print all Parameters

            for (auto &Pair : Parameters)
            {
                std::cout << Pair.first << ": " << Pair.second << std::endl;
            }

            return HTTP::Response::HTML(HTTP::Status::OK, "<h1>Index = " + Parameters["Index"] + "</h1>");
        });

    Server.Listen(20)
        .Start(0)
        .GetInPool();

    return 0;
}