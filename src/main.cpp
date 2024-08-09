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
	if (argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
		return (EXIT_FAILURE);
	}
	const std::string configFile = argv[1];

	Config config(configFile);
	try {
		config.loadConfig();
	} catch (std::invalid_argument& e) {
		std::cerr << "Error: configuration error: " << e.what() << std::endl;
		return 1;
	} catch (std::out_of_range& e) {
		std::cerr << "Error: configuration error: " << e.what() << std::endl;
		return 3;
	} catch (std::overflow_error& e) {
		std::cerr << "Error: configuration error: " << e.what() << std::endl;
		return 4;
	}

	typedef std::map<short, ServerConfig> ConfType;

	const ConfType &allConfigs = config.getAllServerConfigs();
	try {
		clnTmpDir();
	} catch (Server::InitialisationException& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
	std::vector<Server> servers(allConfigs.size());
	size_t i = 0;
	try {
		for (ConfType::const_iterator it = allConfigs.begin(); it != allConfigs.end(); ++it) {
			servers[i++].initEndpoint(it->second.hostnames, it->first, it->second);
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
	return (0);
}
