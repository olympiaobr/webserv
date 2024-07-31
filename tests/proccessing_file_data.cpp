#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <vector>
#include <fstream>
#include <dirent.h>

size_t write_data_to_file(const char *buffer, size_t buffer_size, std::string file_name) {
	DIR *dirp;
	dirent *dir;

	dirp = opendir(".");
	if (!dirp)
		return 0;
	size_t len = file_name.size();
	dir = readdir(dirp);
	// search if file with the same name exists + if chunked file exists
	while (dir) {
		if (dir->d_namlen == len && strcmp(dir->d_name, file_name.c_str()) == 0) {
				(void)closedir(dirp);
				return (0);
		}
		dir = readdir(dirp);
	}
    (void)closedir(dirp);
	int file_fd = open("./temp_name.jpg", O_WRONLY | O_CREAT | O_NONBLOCK | O_APPEND);
	if (write(file_fd, buffer, buffer_size) < 0)
		return 0;
	close(file_fd);
	return 0;
}
