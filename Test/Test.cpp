#include <iostream>
#include <string>

#include "Network/DNS.cpp"
#include "Conversion/Base64.cpp"
#include "Conversion/Hex.cpp"
#include "Base/Test.cpp"
#include "Base/File.cpp"
#include "Cryptography/Digest.cpp"
#include "Cryptography/Random.cpp"
#include "Cryptography/RSA.cpp"

using namespace std;
using namespace Core;

void get_dns_servers()
{
	FILE *fp;
	char line[200], *p;
	if ((fp = fopen("/etc/resolv.conf", "r")) == NULL)
	{
		printf("Failed opening /etc/resolv.conf file \n");
	}

	while (fgets(line, 200, fp))
	{
		if (line[0] == '#')
		{
			continue;
		}

		if (strncmp(line, "nameserver", 10) == 0)
		{
			p = strtok(line, " ");
			p = strtok(NULL, " ");

			cout << p << endl;
		}
	}
}
#define PORT 8888

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

		server.Await(Network::Socket::In);

		int Result = server.ReceiveFrom(buffer, sizeof buffer, Client);

		buffer[Result] = '\0';

		cout << Client << " : " << buffer << endl;

		server.SendTo((char*)"Hello there", 12, Client);
	}

	return 0;
}