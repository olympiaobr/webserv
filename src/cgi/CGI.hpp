#ifndef CGI_HPP
# define CGI_HPP

# include <string>
# include <map>


# include "../utilities/Utils.hpp"
# include "../requests/Request.hpp"

class CGIHandler {
public:
	CGIHandler(const std::string& path, const Request& req);
    ~CGIHandler();
    std::string execute();
    // std::string getInterpreter(const std::string& scriptPath);
	void setupEnvironment();

private:
    std::string scriptPath;
    ServerConfig serverConfig;
    std::map<std::string, std::string> environment;
	const Request& request;
    char** envp;
    void cleanupProcess(pid_t pid, int* pipes);
};

#endif
