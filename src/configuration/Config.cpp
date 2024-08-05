#include "Config.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <algorithm>

Config::Config(const std::string& filename) : _filename(filename) {}

Config::~Config() {}

Config::Config(const Config& other) : _filename(other._filename), _servers(other._servers) {}

Config& Config::operator=(const Config& other) {
    if (this != &other) {
        _filename = other._filename;
        _servers = other._servers;
    }
    return *this;
}

void Config::parseServerConfig(ServerConfig& config, const std::string& line)
{
    std::istringstream iss(line);

    std::string key;
    iss >> key;
    std::string value;
    getline(iss, value);

    size_t start = value.find_first_not_of(" \t\"");
    if (start != std::string::npos) {
        value.erase(0, start);
    }
    size_t end = value.find_last_not_of(" \t;\"");
    if (end != std::string::npos) {
        value.erase(end + 1, value.length() - end);
    }

    if (key == "host" || key == "server_name") {
        config.hostnames.push_back(value);
    }
    else if (key == "root") {
        config.root = value;
    }
    else if (key == "client_max_body_size") {
        config.body_limit = atoi(value.c_str());
    }
    else if (key.find("error_page") == 0) {
        std::istringstream errorPageIss(value);
        int errorCode;
        std::string errorPagePath;
        if (errorPageIss >> errorCode >> errorPagePath) {
            config.error_pages[errorCode] = errorPagePath;
        }
    }
}

void Config::parseRouteConfig(RouteConfig& config, const std::string& line)
{
    std::istringstream iss(line);

    std::string key;
    iss >> key;
    std::string value;
    getline(iss, value);


    size_t start = value.find_first_not_of(" \t\"");
    value.erase(0, start);
    size_t end = value.find_last_not_of(" \t;\"");
    if (end != std::string::npos) {
        value.erase(end + 1);
    }
    std::cout << "Parsing route config: " << key << " = " << value << std::endl;
    if (key == "allow_methods") {
        std::istringstream methods(value);
        std::string method;
        while (methods >> method) {
            method.erase(std::remove(method.begin(), method.end(), ';'), method.end());
            config.allowed_methods.push_back(method);
        }
    }
    else if (key == "index") {
        config.default_file = value;
    }
    else if (key == "root") {
        config.root = value;
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
    std::string currentLocationPath;
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
            iss >> currentLocationPath;
            continue;
        }
        if (key == "}") {
            if (inLocationBlock) {
                inLocationBlock = false;
                currentServerConfig.routes[currentLocationPath] = currentRouteConfig;
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

// Streaming operator for RouteConfig
std::ostream& operator<<(std::ostream& os, const RouteConfig& config) {
    os << "      Root: " << config.root << "\n"
	   << "      Default File: " << config.default_file << "\n"
       << "      Allowed Methods: ";

    for (size_t i = 0; i < config.allowed_methods.size(); ++i) {
        if (i > 0) os << ", ";
        os << config.allowed_methods[i];
    }
    os << "\n";

    return os;
}

// Streaming operator for ServerConfig
std::ostream& operator<<(std::ostream& os, const ServerConfig& config) {
    os << "  Hostnames: ";

    for (size_t i = 0; i < config.hostnames.size(); ++i) {
        if (i > 0) os << ", ";
        os << config.hostnames[i];
    }
    os << "\n"
       << "  Root: " << config.root << "\n"
       << "  Body Limit (MB): " << config.body_limit << "\n"
       << "  Error Pages:\n";

    std::map<int, std::string>::const_iterator it;
    for (it = config.error_pages.begin(); it != config.error_pages.end(); ++it) {
        os << "    " << it->first << ": " << it->second << "\n";
    }

    os << "  Routes:\n";
    std::map<std::string, RouteConfig>::const_iterator route_it;
    for (route_it = config.routes.begin(); route_it != config.routes.end(); ++route_it) {
        os << "    " << route_it->first << ":\n" << route_it->second;
    }

    return os;
}

// Streaming operator for Config
std::ostream& operator<<(std::ostream& os, const Config& config) {
    os << std::endl
	   << "Config:\n"
       << "  Filename: " << config._filename << "\n"
	   << std::endl;

    std::map<short, ServerConfig>::const_iterator it;
    for (it = config.getAllServerConfigs().begin(); it != config.getAllServerConfigs().end(); ++it) {
        os << "Port " << it->first << ":\n" << it->second << std::endl;
    }

    return os;
}