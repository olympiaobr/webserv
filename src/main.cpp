#include "server/Server.hpp"
#include "include/debug.hpp"
#include <dirent.h>
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

#define TEMP_FILES_DIRECTORY "tmp/"
#define MAX_CONNECTIONS 100
#define BACKLOG 3
#define DEFAULT_CONFIG "config/c_file.conf"

void	clnTmpDir() {
	DIR* dir = opendir(TEMP_FILES_DIRECTORY);
	if (dir == NULL) {
		std::string error_msg;
		error_msg += strerror(errno);
		error_msg += ": failed to open directory";
		throw Server::InitialisationException(error_msg.c_str());
	}
	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_type == DT_REG) {
			std::string filePath = std::string(TEMP_FILES_DIRECTORY) + entry->d_name;
			std::remove(filePath.c_str());
		}
	}
	closedir(dir);
}

int	main(int argc, char *argv[])
{
	if (argc > 2)
	{
		std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
		return (EXIT_FAILURE);
	}
	std::string configFile;
	if (argc == 2)
		configFile = argv[1];
	else
		configFile = DEFAULT_CONFIG;
	Config config(configFile);
	try {
		config.loadConfig();
	} catch (std::exception &e) {
        std::cerr << "Error: configuration error: " << e.what() << std::endl;
        return 1;
    }

	const ConfigList &allConfigs = config.getAllServerConfigs();
	try {
		clnTmpDir();
	} catch (Server::InitialisationException& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
	std::vector<Server> servers(allConfigs.size());
	size_t i = 0;
	try {
		for (ConfigList::const_iterator portIt = allConfigs.begin(); portIt != allConfigs.end(); ++portIt)
		{
			const short port = portIt->first;
			const std::vector<ServerConfig>&hostConfigs = portIt->second;
			Server server;
			servers[i++].initEndpoint(port, hostConfigs);
		}
		Server::RUN(servers);
		clnTmpDir();

	} catch (Server::InitialisationException& e) {
		std::cerr << RED << e.what() << " (on exit)" << RESET << std::endl;
		return 1;
	} catch (Server::PollingErrorException& e) {
		std::cerr << RED << e.what() << RESET << std::endl;
		return 2;
	}
}

