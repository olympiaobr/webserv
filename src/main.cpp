#include "configuration/Config.hpp"
#include "server/Server.hpp"
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#define MAX_CONNECTIONS 100
#define BACKLOG 3

int	main(int argc, char *argv[])
{
	if (argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
		return (EXIT_FAILURE);
	}
	const std::string configFile = argv[1];

	Config config(configFile);
	if (!config.loadConfig())
	{
		std::cerr << "Failed to load configuration from " << configFile << std::endl;
		return (EXIT_FAILURE);
	}

	std::cout << config << std::endl;

	typedef std::map<short, ServerConfig> ConfType;

	const ConfType &allConfigs = config.getAllServerConfigs();
	std::vector<Server> servers(allConfigs.size());
	size_t i = 0;
	for (ConfType::const_iterator it = allConfigs.begin(); it != allConfigs.end(); ++it)
	{
		std::cout << "Loaded configuration for port: " << it->first << std::endl;
		servers[i++].initEndpoint(it->second.hostnames, it->first, it->second);
	}
	Server::RUN(servers);

	return (0);
}
