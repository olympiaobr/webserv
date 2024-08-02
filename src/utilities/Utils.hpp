#ifndef UTILS_HPP
# define UTILS_HPP

# include <iostream>
# include <sstream>
# include <fcntl.h>
# include <unistd.h>

namespace utils {
	std::string &toLowerCase(std::string &string);
	std::string buildPath(int socket, const char *folder_name);
	bool checkChunkFileExistance(const std::string &file_name);
	bool deleteFile(const std::string &file_name);
	std::string getFileExtension(const std::string &file);
}

#endif
