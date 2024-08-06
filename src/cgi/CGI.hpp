#ifndef CGI_HPP
#define CGI_HPP

#include <string>
#include <map>
#include "../requests/Request.hpp"
#include "../configuration/Config.hpp"
#include "../utilities/Utils.hpp"

class CGIHandler {
public:
    CGIHandler(const std::string& path, const Request& req, const ServerConfig& conf);
    ~CGIHandler();
    std::string execute();
	void setupEnvironment();
    std::string parseHeaders(std::string& output);

private:
    std::string scriptPath;
    Request request;
    ServerConfig serverConfig;
    std::map<std::string, std::string> environment;
    char** envp;
};

#endif
