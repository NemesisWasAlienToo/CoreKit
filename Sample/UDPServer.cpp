#include <iostream>
#include <string>
#include <thread>

#include "Iterable/List.hpp"
#include "Network/DNS.hpp"
#include "Network/Address.hpp"
#include "Network/Socket.hpp"

using namespace Core;

int main(int argc, char const *argv[])
{
	Network::EndPoint Host(Network::Address(Network::Address::Any()), 8888);

	Network::Socket server(Network::Socket::IPv4, Network::Socket::UDP);

	server.Bind(Host);

	std::cout << Network::DNS::HostName() << " is listenning on " << Host << std::endl;

	while (1)
	{
		char buffer[1024];

		Network::EndPoint Client;

		server.Await(Network::Socket::In);

        std::cout << server.Received() << " Bytes ready" << std::endl;

		int Result = server.ReceiveFrom(buffer, sizeof buffer, Client);

		buffer[Result] = '\0';

		std::cout << Client << " : " << buffer << std::endl;

		server.SendTo((char*)"Hello there", 12, Client);
	}

	return 0;
}