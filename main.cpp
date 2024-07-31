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

#define PORT_1 8080
#define PORT_2 8081
#define BUFFER_SIZE 1024
#define MAX_CONNECTIONS 100
#define BACKLOG 3

int	main(int argc, char *argv[])
{
	HostList	defaultHosts;
	Server		server;
	Server		server2;

	if (argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
		return (EXIT_FAILURE);
	}
	const std::string configFile = argv[1];
	defaultHosts.push_back("www.example.com");
	defaultHosts.push_back("api.example.com");
	defaultHosts.push_back("www.mysite.com");
	std::cout << "Loading configuration from file: " << configFile << std::endl;
	Config config(configFile);
	if (!config.loadConfig())
	{
		std::cerr << "Failed to load configuration from " << configFile << std::endl;
		return (EXIT_FAILURE);
	}
	const std::map<short,
		ServerConfig> &allConfigs = config.getAllServerConfigs();
	for (std::map<short,
		ServerConfig>::const_iterator it = allConfigs.begin(); it != allConfigs.end(); ++it)
	{
		std::cout << "Loaded configuration for port: " << it->first << std::endl;
	}
	try
	{
		const ServerConfig &config8080 = config.getServerConfig(PORT_1);
		const ServerConfig &config8081 = config.getServerConfig(PORT_2);
		std::vector<std::string> hostsForServer1 = config8080.hostnames.empty() ? defaultHosts : config8080.hostnames;
		std::vector<std::string> hostsForServer2 = config8081.hostnames.empty() ? defaultHosts : config8081.hostnames;
		server.initEndpoint(hostsForServer1, PORT_1);
		server.listenPort(BACKLOG);
		server2.initEndpoint(hostsForServer2, PORT_2);
		server2.listenPort(BACKLOG);
		std::cout << "Server 1 is listening on port " << PORT_1 << std::endl;
		std::cout << "Server 2 is listening on port " << PORT_2 << std::endl;
	}
	catch (const std::out_of_range &e)
	{
		std::cerr << "Error: Configuration for a specified port is not found." << std::endl;
		return (EXIT_FAILURE);
	}
	while (true)
	{
		server.pollfds();  // sets revents status to all sockets in list
		server.pollLoop(); // loops through sockets and handles if new request received on socket
		server2.pollfds();
		server2.pollLoop();
	}
	return (0);
}
