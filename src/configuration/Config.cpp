#include "Config.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>

Config::Config(const std::string& filename) : _filename(filename) {}

Config::~Config() {
}

Config::Config(const Config& other) : _filename(other._filename), _servers(other._servers) {}

Config& Config::operator=(const Config& other) {
    if (this != &other) {
        _filename = other._filename;
        _servers = other._servers;
    }
    return *this;
}

void Config::parseServerConfig(ServerConfig& config, const std::string& line) {
    std::istringstream iss(line);

    std::string key;
    iss >> key;
    std::string value;
    getline(iss, value);
    value.erase(0, value.find_first_not_of(" \t"));

	std::cout << "Parsing server config: " << key << " = " << value << std::endl;
    if (key == "host" || key == "server_name") {
        config.hostnames.push_back(value);
    }
    else if (key == "root") {
        config.root = value;
    }
    else if (key == "client_max_body_size") {
        config.body_limit = atoi(value.c_str());
    }
}

void Config::parseRouteConfig(RouteConfig& config, const std::string& line) {
    std::istringstream iss(line);

    std::string key;
    iss >> key;
    std::string value;
    getline(iss, value);
    value.erase(0, value.find_first_not_of(" \t"));

	std::cout << "Parsing route config: " << key << " = " << value << std::endl;
    if (key == "allow_methods") {
        std::istringstream methods(value);

        std::string method;
        while (methods >> method) {
            config.allowed_methods.push_back(method);
        }
    }
    else if (key == "index") {
        config.default_file = value;
    }
}

bool Config::loadConfig() {
    std::ifstream file(_filename.c_str());
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << _filename << std::endl;
        return false;
    }

    std::string line;
    ServerConfig currentServerConfig;
    RouteConfig currentRouteConfig;
    bool inServerBlock = false;
    bool inLocationBlock = false;
    short currentPort = 0;

    while (getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        iss >> key;

        if (key == "server") {
            if (inServerBlock) {
                std::cerr << "Nested server block found, which is not allowed.\n";
                return false;
            }
            inServerBlock = true;
            currentServerConfig = ServerConfig();
            continue;
        }
        if (key == "location") {
            if (!inServerBlock) {
                std::cerr << "Location block outside of server block.\n";
                return false;
            }
            if (inLocationBlock) {
                std::cerr << "Nested location block found, which is not allowed.\n";
                return false;
            }
            inLocationBlock = true;
            currentRouteConfig = RouteConfig();
            continue;
        }
        if (key == "}") {
            if (inLocationBlock) {
                inLocationBlock = false;
                currentServerConfig.routes["/"] = currentRouteConfig;
            } else if (inServerBlock) {
                inServerBlock = false;
                if (currentPort == 0) {
                    std::cerr << "Server block missing 'listen' directive.\n";
                    return false;
                }
                _servers[currentPort] = currentServerConfig;
            }
            continue;
        }
        if (inServerBlock && !inLocationBlock) {
            if (key == "listen") {
                iss >> currentPort;
            } else {
                parseServerConfig(currentServerConfig, line);
            }
        } else if (inLocationBlock) {
            parseRouteConfig(currentRouteConfig, line);
        }
    }
    file.close();
	std::cout << "Final server configurations:\n";
    for (std::map<short, ServerConfig>::const_iterator it = _servers.begin(); it != _servers.end(); ++it) {
        std::cout << "Port: " << it->first << "\n";
        const ServerConfig& config = it->second;
        std::cout << "Root: " << config.root << "\n";
        std::cout << "Hostnames:\n";
        for (size_t i = 0; i < config.hostnames.size(); ++i) {
            std::cout << "  " << config.hostnames[i] << "\n";
        }
    }
    return true;
}

const ServerConfig& Config::getServerConfig(short port) const {
    std::map<short, ServerConfig>::const_iterator it = _servers.find(port);
    if (it == _servers.end()) {
        throw std::out_of_range("Configuration for specified port is not found.");
    }
    return it->second;
}

const std::map<short, ServerConfig>& Config::getAllServerConfigs() const {
    return _servers;
}

void Config::addServerConfig(short port, const ServerConfig& serverConfig) {
    _servers[port] = serverConfig;
}
