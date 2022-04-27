#include <iostream>
#include <string>

#include <Network/DNS.hpp>
#include <Network/HTTP/HTTP.hpp>

using namespace Core;

int main(int argc, char const *argv[])
{
    Network::HTTP::Response Response = Network::HTTP::Request::Get("/").Send(Network::DNS::ResolveSingle("google.com", "http"));

    std::cout << Response.Content << std::endl;

    return 0;
}