#include "Utils.hpp"
#include <cstdio>

std::string &utils::toLowerCase(std::string &string) {
	for (size_t i = 0; i < string.size(); ++i) {
		string[i] = tolower(string[i]);
	}
	return string;
}

bool utils::checkChunkFileExistance(const std::string &file_name) {
	int	fd;

	fd = open(file_name.c_str(), O_RDONLY);
	if (fd < 0)
		return false;
	close(fd);
	return true;
}

std::string utils::buildPath(int socket, const char *folder_name) {
	std::string			client_socket;
	std::string			file_name;
	std::stringstream	ss;

	file_name += folder_name;
	file_name += "/";
	ss << socket;
	ss >> client_socket;
	file_name += client_socket;
	file_name += ".chunked";

	return file_name;
}

bool utils::deleteFile(const std::string &file_name) {
	/*ddavlety*/
	/* IMPORTANT! remove function may be not allowed */
	if (std::remove(file_name.c_str()) == 0)
        return true;
	return false;
}

std::string utils::getFileExtension(const std::string &file) {
    std::string extension;

	size_t  i = file.find_last_of('.');
	extension = file.substr(i + 1);
	return extension;
}

std::string utils::saveFile(const std::string &file_name, const ServerConfig &config, const std::string uri) {
	std::time_t now = std::time(0);
	std::tm* now_tm = std::localtime(&now);

	std::ostringstream oss;
	oss << (now_tm->tm_year + 1900)
        << (now_tm->tm_mon + 1)
        << now_tm->tm_mday
        << now_tm->tm_hour
        << now_tm->tm_min
        << now_tm->tm_sec;

	std::string new_file_name;
	new_file_name += config.root;
	new_file_name += uri;
	oss << "-";
	oss << rand() % 1000;
	new_file_name += oss.str();
	new_file_name += ".file";

	rename(file_name.c_str(), new_file_name.c_str());
	return new_file_name;
}

std::string utils::chunkFileName(int socket) {
	std::stringstream ss;
	std::string file_name;
	std::string socket_name;

	file_name = TEMP_FILES_DIRECTORY;
	ss << socket;
	ss >> socket_name;
	ss.clear();
	file_name += socket_name;
	file_name += ".chunk";
    return file_name;
}

char	*utils::strstr(const char *haystack, const char *needle, ssize_t len) {
	if (!(*needle))
		return ((char *)haystack);
	if (len < 0)
		return (NULL);
	if ((size_t)len < strlen((char *)needle))
		return (NULL);
	while (len != 0)
	{
		if (strncmp((char *)haystack, (char *)needle, strlen((char *)needle)) == 0)
			return ((char *)haystack);
		haystack++;
		len--;
	}
	return (NULL);
}

int utils::stoi(const std::string &str, int sys)
{
	int out;
	std::stringstream ss;

	if (sys == 16) {
		ss << std::hex << str;
	} else {
		ss << str;
	}
	ss >> out;
	return out;
}

bool utils::fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

std::string utils::toString(int value) {
        std::ostringstream oss;
        oss << value;
        return oss.str();
}

std::string utils::decodePercentEncoding(const std::string& encoded) {
    std::ostringstream decoded;
    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            std::string hexValue = encoded.substr(i + 1, 2);
            char decodedChar = static_cast<char>(utils::stoi(hexValue, 16));
            decoded << decodedChar;
            i += 2;
        } else {
            decoded << encoded[i];
        }
    }
    return decoded.str();
}

bool utils::isValidEnvironmentVariable(const std::string &key, const std::string &value)
{
    const size_t maxLength = 1024;
    if (key.length() > maxLength || value.length() > maxLength)
    {
        std::cerr << "Environment variable too long.\n";
        return false;
    }
    if (key.empty() || !(isalpha(key[0]) || key[0] == '_'))
    {
        std::cerr << "Invalid environment variable key format.\n";
        return false;
    }
    for (size_t i = 1; i < key.length(); ++i)
    {
        if (!(isalnum(key[i]) || key[i] == '_'))
        {
            std::cerr << "Invalid environment variable key format.\n";
            return false;
        }
    }
    for (size_t i = 0; i < value.length(); ++i)
    {
        if (value[i] < 32 || value[i] > 126)
        {
            std::cerr << "Invalid environment variable value format.\n";
            return false;
        }
    }
    return true;
}


std::string utils::sanitizeInput(const std::string &input, size_t maxLength)
{
	if (input.length() > maxLength)
	{
		throw std::runtime_error("Input exceeds maximum allowed length.");
	}

	std::string sanitized = input;
	// for (size_t i = 0; i < input.size(); ++i)
	// {
	// 	// Allow alphanumeric and safe symbols only
	// 	if (isalnum(input[i]) || input[i] == '_' || input[i] == '-' || input[i] == '.' || input[i] == '/')
	// 	{
	// 		sanitized += input[i];
	// 	}
	// 	else if (input[i] == ' ')
	// 	{
	// 		sanitized += "%20"; // Encode spaces as %20
	// 	}
	// 	else
	// 	{
	// 		throw std::runtime_error("Invalid character in input.");
	// 	}
	// }
	return sanitized;
}

