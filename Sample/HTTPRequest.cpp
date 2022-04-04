#include <iostream>
#include <string>
#include <unistd.h>

#include <File.hpp>
#include <Network/DNS.hpp>
#include <Network/HTTP/HTTP.hpp>

using namespace Core;

int main(int argc, char const *argv[])
{
    auto result = Network::DNS::Resolve("google.com");

    Network::EndPoint Google(result[0], 80);

    auto Response = Network::HTTP::Send(Google, Network::HTTP::Request::Get("/"));

    std::cout << Response.Content << std::endl;

    return 0;
}