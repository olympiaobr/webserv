#include "CGI.hpp"
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

std::string normalizePath(const std::string& path);

CGIHandler::CGIHandler(const std::string& path, const Request& req)
    : scriptPath(normalizePath(path)), serverConfig(req.getConfig()), request(req) {
    setupEnvironment();
}


CGIHandler::~CGIHandler() {
    if (envp) {
        for (int i = 0; envp[i] != NULL; i++) {
            delete [] envp[i];
        }
        delete[] envp;
    }
}

std::string normalizePath(const std::string& path) {
    std::string result;
    bool lastWasSlash = false;

    for (size_t i = 0; i < path.length(); ++i) {
        char ch = path[i];
        if (ch == '/') {
            if (!lastWasSlash) {
                result += ch;
                lastWasSlash = true;
            }
        } else {
            result += ch;
            lastWasSlash = false;
        }
    }

    // Ensure no trailing slash unless it’s the root "/"
    if (result.size() > 1 && result[result.size() - 1] == '/') {
        result.erase(result.size() - 1);
    }

    return result;
}



void CGIHandler::setupEnvironment() {
    std::string contentLength = utils::toString(request.getBody().length());
    std::string normalizedScriptPath = normalizePath(scriptPath);

    environment["REQUEST_METHOD"] = request.getMethod();
    environment["SCRIPT_FILENAME"] = normalizedScriptPath;
    environment["QUERY_STRING"] = request.getQueryString();
    environment["CONTENT_LENGTH"] = contentLength;
    environment["CONTENT_TYPE"] = (request.getMethod() == "POST") ? "application/x-www-form-urlencoded" : request.getHeader("Content-Type");
    std::string serverPort = (serverConfig.port > 0) ? utils::toString(serverConfig.port) : "8000";
    environment["SERVER_PORT"] = serverPort;
    environment["SERVER_PROTOCOL"] = "HTTP/1.1";

    std::string uri = request.getUri();
    size_t pathInfoPos = uri.find(scriptPath);
    environment["PATH_INFO"] = (pathInfoPos != std::string::npos && pathInfoPos + scriptPath.length() < uri.length())
                                ? uri.substr(pathInfoPos + scriptPath.length())
                                : "/";
	environment["REDIRECT_STATUS"] = "200";
    environment["GATEWAY_INTERFACE"] = "CGI/1.1";
    environment["SERVER_SOFTWARE"] = "webserv/1.0";

    std::cerr << "Setting up environment variables for CGI execution:\n";
    for (std::map<std::string, std::string>::iterator it = environment.begin(); it != environment.end(); ++it) {
        std::cerr << it->first << "=" << it->second << std::endl;
    }

    std::vector<std::string> envStrings;
    envp = new char*[environment.size() + 1];
    int i = 0;
    for (std::map<std::string, std::string>::const_iterator it = environment.begin(); it != environment.end(); ++it) {
        std::string env = it->first + "=" + it->second;
        envStrings.push_back(env);
        envp[i] = new char[env.size() + 1];
        std::copy(env.begin(), env.end(), envp[i]);
        envp[i][env.size()] = '\0';
        i++;
    }
    envp[i] = NULL;
}

std::string CGIHandler::getInterpreter(const std::string& scriptPath) {
    const std::string pythonInterpreter = "./web/cgi/.venv/bin/python3";

    size_t extPos = scriptPath.find_last_of(".");
    if (extPos != std::string::npos) {
        std::string ext = scriptPath.substr(extPos);
        if (ext == ".py") {
            return pythonInterpreter;
        }
    }
    return "";
}

std::string CGIHandler::execute() {
    int pipe_fds[2];
    if (pipe(pipe_fds) != 0) {
        std::cerr << "Failed to create pipe: " << strerror(errno) << std::endl;
        throw std::runtime_error("Failed to create pipe");
    }

    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "Failed to fork process: " << strerror(errno) << std::endl;
        throw std::runtime_error("Failed to fork process");
    }

    if (pid == 0) {  // Child process
        close(pipe_fds[0]);  // Close the read end in the child
        if (dup2(pipe_fds[1], STDOUT_FILENO) == -1) {
            std::cerr << "dup2 failed: " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }
        close(pipe_fds[1]);  // No longer need this after dup2

        std::string interpreter = getInterpreter(scriptPath);
        if (interpreter.empty()) {
            std::cerr << "No interpreter found for script: " << scriptPath << std::endl;
            exit(EXIT_FAILURE);
        }

        char* const args[] = {
            const_cast<char*>(interpreter.c_str()),
            const_cast<char*>(scriptPath.c_str()),
            NULL
        };
        std::cerr << "Executing CGI with interpreter: " << interpreter << ", script: " << scriptPath << std::endl;
        for (int i = 0; envp[i] != NULL; ++i) {
            std::cerr << "envp[" << i << "]: " << envp[i] << std::endl;
        }
        execve(interpreter.c_str(), args, envp);
        perror("execve failed");
        exit(EXIT_FAILURE);
    } else {  // Parent process
        close(pipe_fds[1]);  // Close the write end in the parent
        std::string output;
        char buffer[1024];
        ssize_t bytesRead;

        while ((bytesRead = read(pipe_fds[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0';
            output.append(buffer);
        }
        close(pipe_fds[0]);
        int status;
        waitpid(pid, &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            std::cerr << "CGI exited with status: " << WEXITSTATUS(status) << std::endl;
            throw std::runtime_error("Failed to run CGI");
        }
        return output;
    }
}
