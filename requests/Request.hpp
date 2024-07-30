#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>
#include <iostream>

class Request {
private:
    int _clientSocket;
    std::string _method;
    std::string _uri;
    std::string _httpVersion;
    std::map<std::string, std::string> _headers;
    std::string _body;

    void _parseRequestLine(const std::string& line);
    void _parseHeader(const std::string& line);
    void _readBody(int contentLength);

public:
    Request(int clientSocket);
    ~Request();

    bool parse();

    std::string getMethod() const;
    std::string getUri() const;
    std::string getHttpVersion() const;
    std::string getHeader(const std::string& key) const;
    const std::map<std::string, std::string>& getHeaders() const;
    std::string getBody() const;

    friend std::ostream& operator<<(std::ostream& os, const Request& request);
};

#endif // REQUEST_HPP