#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>

class Response {
private:
    int _statusCode;
    std::string _statusMessage;
    std::string _contentType;
    std::string _content;
    std::string _buffer;

    void _buildBuffer();

public:
    Response();
    Response(int statusCode, const std::string& statusMessage, const std::string& content, const std::string& contentType = "text/plain");

    void setStatus(int statusCode, const std::string& statusMessage);
    void setContent(const std::string& content);
    void setContentType(const std::string& contentType);

    const char* build();
    size_t getSize() const;
};

#endif // RESPONSE_HPP