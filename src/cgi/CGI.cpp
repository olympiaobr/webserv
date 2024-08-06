#include "CGI.hpp"
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <iostream>
#include <fstream>
#include <sstream>

CGIHandler::CGIHandler(const std::string& path, const Request& req, const ServerConfig& conf)
    : scriptPath(path), request(req), serverConfig(conf) {
    setupEnvironment();
}

CGIHandler::~CGIHandler() {
    if (envp) {
        for (int i = 0; envp[i] != NULL; i++) {
            free(envp[i]);
        }
        delete[] envp;
    }
}

void CGIHandler::setupEnvironment() {
    char contentLength[20];
    sprintf(contentLength, "%ld", request.getBody().length());
    environment["REQUEST_METHOD"] = request.getMethod();
    environment["SCRIPT_NAME"] = scriptPath;
    environment["QUERY_STRING"] = request.getQueryString();
    environment["CONTENT_LENGTH"] = contentLength;
    environment["CONTENT_TYPE"] = request.getHeader("Content-Type");
    environment["REMOTE_ADDR"] = request.getClientIPAddress();
    environment["SERVER_NAME"] = serverConfig.hostnames[0];
    char serverPort[10];
    sprintf(serverPort, "%d", serverConfig.port);
    environment["SERVER_PORT"] = serverPort;
    environment["SERVER_PROTOCOL"] = "HTTP/1.1";
    environment["PATH_INFO"] = request.getUri();

    std::vector<std::string> envStrings;
    envp = new char*[environment.size() + 1];
    int i = 0;
    for (std::map<std::string, std::string>::iterator it = environment.begin(); it != environment.end(); ++it) {
        std::string env = it->first + "=" + it->second;
        envStrings.push_back(env);
        envp[i++] = strdup(envStrings.back().c_str());
    }
    envp[i] = NULL;
}

std::string CGIHandler::execute() {
    int fd[2];
    if (pipe(fd) != 0) {
        throw std::runtime_error("Failed to create pipe");
    }

    pid_t pid = fork();
    if (pid == -1) {
        throw std::runtime_error("Failed to fork process");
    } else if (pid == 0) {
        // Child process
        close(fd[0]); // Close read end
        dup2(fd[1], STDOUT_FILENO); // Redirect stdout to pipe

        // Convert environment map to suitable format for execve
        char** envp = new char*[environment.size() + 1];
        int i = 0;
        std::map<std::string, std::string>::const_iterator it;
        for (it = environment.begin(); it != environment.end(); ++it) {
            std::string env = it->first + "=" + it->second;
            envp[i++] = strdup(env.c_str());
        }
        envp[i] = NULL;

        char* argv[2];
        argv[0] = const_cast<char*>(scriptPath.c_str());
        argv[1] = NULL;

        execve(scriptPath.c_str(), argv, envp);
        exit(EXIT_FAILURE); // Exit if execve fails
    } else {
        // Parent process
        close(fd[1]); // Close write end
        int status;
        waitpid(pid, &status, 0);

        // Read output from child process
        std::string output;
        char buffer[1024];
        ssize_t bytesRead;
        while ((bytesRead = read(fd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0';
            output += buffer;
        }
        close(fd[0]);

        return parseHeaders(output);
    }
}

std::string CGIHandler::parseHeaders(std::string& output) {
    // Simple header parsing - adjust as needed
    std::istringstream stream(output);
    std::string headers;
    std::string body;
    std::string line;
    while (std::getline(stream, line) && line != "\r") {
        headers += line + "\n";
    }
    while (std::getline(stream, line)) {
        body += line + "\n";
    }
    return body; // Return body part after headers
}

