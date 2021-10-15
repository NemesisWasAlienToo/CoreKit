#include <iostream>
#include <string>
#include <cstring>

#include "Iterable/List.cpp"
#include "Iterable/Buffer.cpp"
#include "Network/DNS.cpp"
#include "Network/Socket.cpp"
#include "Network/HTTP.cpp"
#include "Cryptography/Digest.cpp"
#include "Cryptography/Random.cpp"

using namespace Core;

Iterable::List<Network::EndPoint> Peers(1);

uint8_t GetRandom(uint8_t Lower, uint8_t Upper){
    
    if(Lower == Upper) return Upper;

    uint8_t Num = 0;

    Cryptography::Random::Bytes((unsigned char *)&Num, sizeof Num);

    while (Num > Upper || Num < Lower)
    {
        Cryptography::Random::Bytes((unsigned char *)&Num, sizeof Num);
    }
    
    return Num;
}

int main(int argc, char const *argv[])
{
    Cryptography::Random::Load(Cryptography::Random::random, Cryptography::Random::InitEntropy());

    // Cryptography::Random::Load();

    Iterable::Buffer Buffer;

    auto result = Network::DNS::Host(Network::Address::IPv4);

    Network::EndPoint Me(result[0], 8888);

    Peers.Add(Me);

    std::cout << Network::DNS::HostName() << " is5 at " << Me << std::endl;

    Network::Socket Server(Network::Socket::IPv4, Network::Socket::TCP);

    Server.Bind(Network::Address::Any(Network::Address::IPv4), 8888);

    Server.Listen(10);

    while (true)
    {
        Server.Await(Network::Socket::In);

        Network::EndPoint Info;

        Network::Socket Client = Server.Accept(Info);

        Peers.Add(Info);

        std::cout << Info << " Added" << std::endl;

        uint8_t RandIndex = GetRandom(0, Peers.Length());

        Client << Peers[RandIndex].ToString() << "\n";

        // Client << Peers.ToString();

        Client.Close();
    }

    return 0;
}

// int main(int argc, char const *argv[])
// {
//     std::cout << "Running on " << Network::DNS::HostName() << std::endl;

//     auto result = Network::DNS::Resolve("google.com");

//     Network::EndPoint Google(result[0], 80);

//     std::cout << "Google is at " << Google << std::endl;

//     Network::Socket client(Network::Socket::IPv4, Network::Socket::TCP);

//     client.Connect(Google);

//     Iterable::Buffer Buffer;

// 	Network::HTTP::Request Req;

// 	Req.Type = "GET";
// 	Req.Version = "1.1";
// 	Req.Headers.insert(std::make_pair<std::string, std::string>("Host", "ConfusionBox"));
// 	Req.Headers.insert(std::make_pair<std::string, std::string>("Connection", "closed"));

//     Buffer << Req;

//     while (!Buffer.IsEmpty())
//     {
//         client << Buffer;

//         client.Await(Network::Socket::Out);
//     }

//     for (client.Await(Network::Socket::In); client.Received() > 0; client.Await(Network::Socket::In, 3000))
//     {
//         std::cout << client;
//     }

//     client.Close();

//     return 0;
// }

// void get_dns_servers()
// {
// 	FILE *fp;
// 	char line[200], *p;
// 	if ((fp = fopen("/etc/resolv.conf", "r")) == NULL)
// 	{
// 		printf("Failed opening /etc/resolv.conf file \n");
// 	}

// 	while (fgets(line, 200, fp))
// 	{
// 		if (line[0] == '#')
// 		{
// 			continue;
// 		}

// 		if (strncmp(line, "nameserver", 10) == 0)
// 		{
// 			p = strtok(line, " ");
// 			p = strtok(NULL, " ");

// 			cout << p << endl;
// 		}
// 	}
// }