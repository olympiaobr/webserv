#include "Utils.hpp"
#include <stdio.h>

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
namespace utils {
    std::string generateDirectoryListing(const std::string& directoryPath) {
        std::ostringstream listing;
        DIR* dir = opendir(directoryPath.c_str());
        if (dir == NULL) {
            return "Error opening directory"; // Handle directory opening errors
        }

        struct dirent* entry;
        listing << "<html><head><title>Index of " << directoryPath
                << "</title></head><body><h1>Index of " << directoryPath
                << "</h1><ul>";
        while ((entry = readdir(dir)) != NULL) {
            listing << "<li><a href=\"" << entry->d_name << "\">" << entry->d_name << "</a></li>";
        }
        closedir(dir);
        listing << "</ul></body></html>";
        return listing.str();
    }
}
