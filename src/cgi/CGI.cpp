#include "cgi.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>

CGI::CGI(const std::string& scriptPath, const std::map<std::string, std::string>& requestHeaders, const std::string& requestBody)
    : scriptPath(scriptPath), requestHeaders(requestHeaders), requestBody(requestBody) {}

CGI::~CGI() {
    cleanupEnvironment();
}

void CGI::setEnvironment() {
    clearenv();

    // Set up CGI-required environment variables
    for (const auto& header : requestHeaders) {
        std::string envKey = "HTTP_" + header.first;
        std::replace(envKey.begin(), envKey.end(), '-', '_');
        std::transform(envKey.begin(), envKey.end(), envKey.begin(), ::toupper);
        setenv(envKey.c_str(), header.second.c_str(), 1);
    }

    // Special environment variables that CGI scripts expect
    setenv("REQUEST_METHOD", requestHeaders.at("Method").c_str(), 1);
    setenv("QUERY_STRING", requestHeaders.at("Query-String").c_str(), 1);
    setenv("CONTENT_LENGTH", requestHeaders.at("Content-Length").c_str(), 1);
    setenv("SCRIPT_FILENAME", scriptPath.c_str(), 1);
    setenv("SCRIPT_NAME", scriptPath.c_str(), 1);
    setenv("PATH_INFO", scriptPath.c_str(), 1);
}

std::string CGI::execute() {
    setEnvironment();

    int outputPipe[2];
    pipe(outputPipe);
    pid_t pid = fork();

    if (pid == 0) {
        // Child process
        dup2(outputPipe[1], STDOUT_FILENO);
        close(outputPipe[0]);
        if (!requestBody.empty()) {
            // If there's a body, send it to the script via stdin
            int inputPipe[2];
            pipe(inputPipe);
            write(inputPipe[1], requestBody.c_str(), requestBody.length());
            close(inputPipe[1]);
            dup2(inputPipe[0], STDIN_FILENO);
        }

        execl(scriptPath.c_str(), scriptPath.c_str(), (char*)NULL);
        exit(EXIT_FAILURE);  // execl only returns if there is an error
    } else {
        // Parent process
        close(outputPipe[1]);
        std::string output = readOutput(outputPipe[0]);
        waitpid(pid, NULL, 0);
        return output;
    }
}

std::string CGI::readOutput(int fileDescriptor) {
    std::ostringstream oss;
    char buffer[1024];
    int nRead;
    while ((nRead = read(fileDescriptor, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[nRead] = '\0';
        oss << buffer;
    }
    close(fileDescriptor);
    return oss.str();
}

void CGI::cleanupEnvironment() {
    // This function might unset all the environment variables we set,
    // or handle other cleanup tasks
}


