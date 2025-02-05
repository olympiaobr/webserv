#include "CGI.hpp"
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <signal.h>
#include <sys/select.h>

std::string normalizePath(const std::string& path);

CGIHandler::CGIHandler(const std::string& path, const Request& req)
    : scriptPath(normalizePath(path)), serverConfig(*req.getConfig()), request(req) {
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
    // std::string contentLength = utils::toString(request.getBody().length());
    std::string contentLength = request.getHeader("Content-Length");
    std::string normalizedScriptPath = normalizePath(scriptPath);

    environment["REQUEST_METHOD"] = request.getMethod();
    environment["SCRIPT_FILENAME"] = normalizedScriptPath;
    environment["QUERY_STRING"] = utils::sanitizeInput(request.getQueryString(), 2048);
    if (environment["QUERY_STRING"]  == "" && request.getMethod() == "DELETE")
    {
        size_t lastSlash = request.getUri().rfind('/');
        if (lastSlash != std::string::npos && lastSlash + 1 < request.getUri().length()) {
            environment["QUERY_STRING"] = "file=" + request.getUri().substr(lastSlash + 1);
        }
    }
    if (scriptPath.find("save_chunks.py") == std::string::npos) {
        environment["CONTENT_LENGTH"] = request.getHeader("Content-Length");
    }
    environment["CONTENT_TYPE"] = request.getHeader("Content-Type");
    std::string serverPort = (serverConfig.port > 0) ? utils::toString(serverConfig.port) : "8000";
    environment["SERVER_PORT"] = serverPort;
    environment["SERVER_PROTOCOL"] = "HTTP/1.1";
    environment["UPLOAD_DIR"] = request.getRouteConfig()->root;
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

    // std::vector<std::string> envStrings;
    envp = new char*[environment.size() + 1];
    int i = 0;
    for (std::map<std::string, std::string>::const_iterator it = environment.begin(); it != environment.end(); ++it) {
        std::string env = it->first + "=" + it->second;
        // envStrings.push_back(env);
        if (!utils::isValidEnvironmentVariable(it->first, it->second))
            throw std::runtime_error("Environment variable failed validation.");
        envp[i] = new char[env.size() + 1];
        std::copy(env.begin(), env.end(), envp[i]);
        envp[i][env.size()] = '\0';
        i++;
    }
    envp[i] = NULL;

    // Add HTTP_COOKIE environment variable
    std::string cookies = request.getHeader("cookie");
    if (!cookies.empty()) {
        environment["HTTP_COOKIE"] = cookies;
    }
}

// std::string CGIHandler::getInterpreter(const std::string& scriptPath) {
//     const std::string pythonInterpreter = "./web/cgi/.venv/bin/python3";

//     size_t extPos = scriptPath.find_last_of(".");
//     if (extPos != std::string::npos) {
//         std::string ext = scriptPath.substr(extPos);
//         if (ext == ".py") {
//             return pythonInterpreter;
//         }
//     }
//     return "";
// }

void CGIHandler::cleanupProcess(pid_t pid, int* pipes) {
    if (pipes[0] != -1) close(pipes[0]);
    if (pipes[1] != -1) close(pipes[1]);
    if (pipes[2] != -1) close(pipes[2]);
    if (pipes[3] != -1) close(pipes[3]);

    if (pid > 0) {
        kill(pid, SIGTERM);
        usleep(100000);
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
    }
}

std::string CGIHandler::execute() {
    int pipes[4] = {-1, -1, -1, -1};
    pid_t pid = -1;

    try {
        if (pipe((int*)&pipes[0]) != 0) { // pipe_child_to_parent
            throw std::runtime_error("Failed to create pipe: " + std::string(strerror(errno)));
        }
        if (pipe((int*)&pipes[2]) != 0) { // pipe_parent_to_child
            throw std::runtime_error("Failed to create pipe");
        }

        pid = fork();
        if (pid < 0) {
            throw std::runtime_error("Failed to fork process");
        }

        if (pid == 0) {  // Child process
            dup2(pipes[2], STDIN_FILENO);  // Read from parent
            dup2(pipes[1], STDOUT_FILENO); // Write to parent

            // Close all pipe ends in child
            close(pipes[0]);
            close(pipes[1]);
            close(pipes[2]);
            close(pipes[3]);

            char* const args[] = {
                const_cast<char*>(scriptPath.c_str()),
                NULL
            };

            execve(scriptPath.c_str(), args, envp);
            perror("execve failed");
            exit(EXIT_FAILURE);
        }

        // Parent process
        close(pipes[1]); // Close write end of child_to_parent
        close(pipes[2]); // Close read end of parent_to_child
        pipes[1] = -1;
        pipes[2] = -1;

        // Write request body
        write(pipes[3], request.getBuffer(), atoi(request.getHeader("Content-Length").c_str()));
        close(pipes[3]); // Close write end after writing
        pipes[3] = -1;

        std::string output;
        char buffer[1024];
        fd_set readfds;
        struct timeval timeout;
        bool hasTimedOut = false;

        while (!hasTimedOut) {
            FD_ZERO(&readfds);
            FD_SET(pipes[0], &readfds);

            timeout.tv_sec = 5;
            timeout.tv_usec = 0;

            int ready = select(pipes[0] + 1, &readfds, NULL, NULL, &timeout);

            if (ready == -1) {
                if (errno == EINTR) continue;
                throw std::runtime_error("Select failed");
            }

            if (ready == 0) {
                hasTimedOut = true;
                break;
            }

            ssize_t bytesRead = read(pipes[0], buffer, sizeof(buffer) - 1);
            if (bytesRead <= 0) break;

            buffer[bytesRead] = '\0';
            output.append(buffer);

            // Basic output size check
            if (output.length() > 1048576) { // 1MB limit
                throw std::runtime_error("CGI output too large");
            }
        }

        // Close remaining pipe
        close(pipes[0]);
        pipes[0] = -1;

        if (hasTimedOut) {
            cleanupProcess(pid, pipes);
            throw std::runtime_error("CGI script execution timed out");
        }

        // Wait for process with timeout
        int status;
        int waitAttempts = 0;
        bool processExited = false;

        while (waitAttempts < 50) { // 5 seconds total (50 * 100000 microseconds)
            pid_t result = waitpid(pid, &status, WNOHANG);
            if (result == pid) {
                processExited = true;
                break;
            }
            if (result == -1) {
                throw std::runtime_error("Wait for child process failed");
            }
            usleep(100000); // 100ms sleep
            ++waitAttempts;
        }

        if (!processExited) {
            cleanupProcess(pid, pipes);
            throw std::runtime_error("CGI script did not exit properly");
        }

        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            cleanupProcess(pid, pipes);
            if (!output.empty()) {
                throw std::runtime_error(output);
            } else {
                throw std::runtime_error("CGI script failed with no output");
            }
        }

        return output;

    } catch (const std::exception& e) {
        cleanupProcess(pid, pipes);
        throw;
    }

    return ""; // Should never reach here
}
