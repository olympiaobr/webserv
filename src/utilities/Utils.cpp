#include "Utils.hpp"

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
	/* IMPORTANT! remove function may be not allowed */
	if (remove(file_name.c_str()) == 0)
        return true;
	return false;
}

std::string utils::getFileExtension(const std::string &file)
{
    std::string extension;

	size_t  i = file.find_last_of('.');
	extension = file.substr(i + 1);
	return extension;
}
