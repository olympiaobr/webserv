#ifndef CGI_HPP
# define CGI_HPP

# include <string>
# include <map>


# include "../requests/Request.hpp"
# include "../responses/Response.hpp"
# include "../configuration/Config.hpp"
# include "../utilities/Utils.hpp"
// # include "../server/Server.hpp"


class Request;

class CGIHandler {
public:
	CGIHandler(const std::string& path, const Request& req);
    ~CGIHandler();
    std::string execute();
	void setupEnvironment();

private:
    std::string scriptPath;
    ServerConfig serverConfig;
    std::map<std::string, std::string> environment;
	const Request& request;
    char** envp;
};

#endif
