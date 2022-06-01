#include <iostream>
#include <string>
#include <unordered_map>

#include <Format/Stringifier.hpp>
#include <Network/HTTP/Server.hpp>

using namespace Core::Network;

int main(int argc, char const *argv[])
{
    HTTP::Server Server({"0.0.0.0:8888"}, {5, 0}, 1);

    Server.SetDefault(
        "/",
        [](EndPoint const &, HTTP::Request const &Request, std::unordered_map<std::string, std::string> &Parameters)
        {
            // @todo Store route arguments in a tuple.
            // @todo Make argument names a compile time template parameter accessing the tuple to get access timeto O(1).

            Request.Cookies(Parameters);

            for (auto &[Key, Value] : Parameters)
            {
                std::cout << Key << ": " << Value << std::endl;
            }

            return HTTP::Response::Text(Request.Version, HTTP::Status::NotFound, "This path does not exist");
        });

    Server.GET(
        "/Static/[Folder]",
        [](EndPoint const &, HTTP::Request const &Request, std::unordered_map<std::string, std::string> &Parameters)
        {
            Iterable::Queue<char> Queue;
            Format::Stringifier Stringifier(Queue);

            Stringifier << "<h1>Folder = " << Parameters["Folder"] << "</h1>"
                        << "<h1>Extension = " << Parameters["Ext"] << "</h1>";

            return HTTP::Response::HTML(Request.Version, HTTP::Status::OK, Stringifier.ToString());
        },
        "Ext");

    Server.GET(
        "/Home",
        [](EndPoint const &, HTTP::Request const &Request, std::unordered_map<std::string, std::string> &Parameters)
        {
            Request.FormData(Parameters);

            // print all Parameters

            for (auto &[Key, Value] : Parameters)
            {
                std::cout << Key << ": " << Value << std::endl;
            }

            return HTTP::Response::Redirect(Request.Version, "/Home/1", {{"test", "test2"}});
        });

    Server.GET(
        "/Home/[Index]",
        [&](EndPoint const &, HTTP::Request const &Request, std::unordered_map<std::string, std::string> &Parameters)
        {
            Iterable::Queue<char> Queue;
            Format::Stringifier Stringifier(Queue);

            Stringifier << "<h1>Index = " << Parameters["Index"] << "</h1>";

            return HTTP::Response::HTML(Request.Version, HTTP::Status::OK, Stringifier.ToString())
                .SetCookie("Name", "TestName", DateTime::FromNow(Duration(60, 0)))
                .SetCookie("Family", "TestFamily", 60)
                .SetCookie("Id", "TestFamily");
        });

    Server.POST(
        "/Home/[Index]",
        [](EndPoint const &, HTTP::Request const &Request, std::unordered_map<std::string, std::string> &Parameters)
        {
            return HTTP::Response::HTML(Request.Version, HTTP::Status::OK, "<h1>Index = " + Parameters["Index"] + "</h1>");
        });

    Server.Run();

    Server.GetInPool();

    // while (true)
    // {
    //     std::string input;

    //     std::cin >> input;
    // }

    // Server.Stop();

    return 0;
}