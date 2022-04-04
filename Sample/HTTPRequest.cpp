#include <iostream>
#include <string>
#include <unistd.h>

#include <File.hpp>
#include <Network/DNS.hpp>
#include <Network/HTTP/HTTP.hpp>

using namespace Core;

int main(int argc, char const *argv[])
{
    Network::HTTP::Response Response = Network::HTTP::Send(Network::DNS::ResolveSingle("google.com", "http"), Network::HTTP::Request::Get("/"));

    std::cout << Response.Content << std::endl;

    return 0;
}