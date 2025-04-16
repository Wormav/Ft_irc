#include <iostream>
#include <cstdlib>
#include <Server.hpp>
#include <cstdlib>

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
		return 1;
	}

	int port = std::atoi(argv[1]);
	std::string password = argv[2];

	Server server(port, password);
	server.run();

	return 0;
}
