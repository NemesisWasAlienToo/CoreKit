#include <iostream>
#include <string>

#include <Format/Stringifier.hpp>
#include <Network/HTTP/Server.hpp>

using namespace Core::Network;

int main(int argc, char const *argv[])
{
    HTTP::Server Server({"0.0.0.0:8888"}, {5, 0});

    Server.SetDefault(
        "/",
        [](EndPoint const &, HTTP::Request const &Request, std::map<std::string, std::string> &Parameters)
        {
            Request.Cookies(Parameters);

            for (auto &[Key, Value] : Parameters)
            {
                std::cout << Key << ": " << Value << std::endl;
            }

            return HTTP::Response::Text(HTTP::HTTP10, HTTP::Status::NotFound, "This path does not exist");
        });

    Server.GET(
        "/Home",
        [](EndPoint const &, HTTP::Request const &Request, std::map<std::string, std::string> &Parameters)
        {
            Request.FormData(Parameters);

            // print all Parameters

            for (auto &[Key, Value] : Parameters)
            {
                std::cout << Key << ": " << Value << std::endl;
            }

            return HTTP::Response::Redirect(HTTP::HTTP10, "/Home/1", {{"test", "test2"}});
        });

    Server.GET(
        "/Home/[Index]",
        [](EndPoint const &, HTTP::Request const &Request, std::map<std::string, std::string> &Parameters)
        {
            Iterable::Queue<char> Queue;
            Format::Stringifier Stringifier(Queue);

            Stringifier << "<h1>Index = " << Parameters["Index"] << "</h1>";

            return HTTP::Response::HTML(HTTP::HTTP10, HTTP::Status::OK, Stringifier.ToString())
                .SetCookie("Name", "TestName", DateTime::FromNow(Duration(60, 0)))
                .SetCookie("Family", "TestFamily", 60)
                .SetCookie("Id", "TestFamily");
        });

    Server.POST(
        "/Home/[Index]",
        [](EndPoint const &, HTTP::Request const &Request, std::map<std::string, std::string> &Parameters)
        {
            return HTTP::Response::HTML(HTTP::HTTP10, HTTP::Status::OK, "<h1>Index = " + Parameters["Index"] + "</h1>");
        });

    Server.Listen(20)
        .Start(0)
        .GetInPool();

    return 0;
}