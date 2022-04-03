#include <iostream>
#include <string>
#include <unistd.h>

#include <File.hpp>
#include <Network/HTTP/Server.hpp>

using namespace Core;

int main(int argc, char const *argv[])
{
    Network::HTTP::Server Server({"0.0.0.0:8888"}, {5, 0});

    Server.OnRequest = [](Network::HTTP::Request &Request, Network::HTTP::Response &Response)
    {
        std::cout << Request.ToString() << std::endl;

        Response.Status = Network::HTTP::Status::OK;

        Response.Headers["Content-Type"] = "text/html; charset=UTF-8";

        Response.Content = "<h1>Hi</h1>";
    };

    Server.Listen(20);

    Server.Start(1);

    Server.GetInPool();
    
    return 0;
}