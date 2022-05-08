#include <iostream>
#include <string>

#include <Network/DNS.hpp>
#include <Network/HTTP/HTTP.hpp>
#include <Network/HTTP/Response.hpp>
#include <Network/HTTP/Request.hpp>

using namespace Core;

int main(int argc, char const *argv[])
{
    Network::HTTP::Response Response = Network::HTTP::Request::Get(Network::HTTP::Request::HTTP10, "/").Send(Network::DNS::ResolveSingle("google.com", "http"));

    std::cout << Response.ToString() << std::endl;

    return 0;
}