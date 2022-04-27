#include <iostream>
#include <string>
#include <tuple>
#include <functional>

#include <Network/HTTP/Server.hpp>

using namespace Core;

int main(int argc, char const *argv[])
{
    Network::HTTP::Server Server({"0.0.0.0:8888"}, {5, 0});

    Server.SetDefault(
        "/",
        [](const Network::EndPoint &, const Network::HTTP::Request &, std::map<std::string, std::string> &)
        {
            return Network::HTTP::Response::Text(Network::HTTP::Status::NotFound, "This path does not exist");
        });

    Server.GET(
        "/Home",
        [](const Network::EndPoint &, const Network::HTTP::Request &Request, std::map<std::string, std::string> &Parameters)
        {
            return Network::HTTP::Response::Redirect("/Home/1");
        });

    Server.POST(
        "/Home/[Index]",
        [](const Network::EndPoint &, const Network::HTTP::Request &Request, std::map<std::string, std::string> &Parameters)
        {
            Network::HTTP::FormData(Request, Parameters);

            // print all Parameters

            for (auto &Pair : Parameters)
            {
                std::cout << Pair.first << ": " << Pair.second << std::endl;
            }

            return Network::HTTP::Response::HTML(Network::HTTP::Status::OK, "<h1>Index = " + Parameters["Index"] + "</h1>");
        });

    Server.Listen(20)
        .Start(0)
        .GetInPool();

    return 0;
}