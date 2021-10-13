#include <iostream>
#include <string>
#include <thread>

#include "Iterable/List.cpp"
#include "Network/DNS.cpp"
#include "Network/Address.cpp"
#include "Network/Socket.cpp"

int main(int argc, char const *argv[])
{
	Core::Network::EndPoint Host(Core::Network::Address(Core::Network::Address::Any()), 8888);

	Core::Network::Socket server(Core::Network::Socket::IPv4, Core::Network::Socket::UDP);

	server.Bind(Host);

	std::cout << Core::Network::DNS::HostName() << " is listenning on " << Host << std::endl;

	while (1)
	{
		char buffer[1024];

		Core::Network::EndPoint Client;

		server.Await(Core::Network::Socket::In);

        std::cout << server.Received() << " Bytes ready" << std::endl;

		int Result = server.ReceiveFrom(buffer, sizeof buffer, Client);

		buffer[Result] = '\0';

		std::cout << Client << " : " << buffer << std::endl;

		server.SendTo((char*)"Hello there", 12, Client);
	}

	return 0;
}