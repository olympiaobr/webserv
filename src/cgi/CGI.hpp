#ifndef CGI_HPP
#define CGI_HPP

#include <string>
#include <map>

class CGI {
public:
    CGI(const std::string& scriptPath, const std::map<std::string, std::string>& requestHeaders, const std::string& requestBody);
    ~CGI();

    std::string execute();

private:
    std::string scriptPath;
    std::map<std::string, std::string> requestHeaders;
    std::string requestBody;
    std::map<std::string, std::string> environmentVariables;

    void setEnvironment();
    std::string readOutput(int fileDescriptor);
    void cleanupEnvironment();
};

#endif
