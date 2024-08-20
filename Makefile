CC := c++
CPPFLAGS := -Wall -Wextra -Werror -std=c++98 -g -fno-limit-debug-info # -fsanitize=address
TARGET := ./webserv
INCLUDES :=	-I./src/server \
 			-I./src/responses \
 			-I./src/requests \
			-I./src/configuration \
			-I./src/utilities \
			-I./src/cgi
RM := rm -rf
SRC_DIR := src/
OBJ_DIR := obj/
SOURCE :=	main.cpp \
 			server/Server.cpp \
 			responses/Response.cpp \
 			requests/Request.cpp \
 			configuration/Config.cpp \
			utilities/Utils.cpp \
			cgi/CGI.cpp

TMPDIR = tmp

SRC := $(addprefix $(SRC_DIR), $(SOURCE))
OBJ := $(addprefix $(OBJ_DIR), $(SOURCE:.cpp=.o))

all: $(TARGET) $(TMPDIR) cgi_deps

$(TMPDIR):
	mkdir -p $(TMPDIR)

cgi_deps:
	pip3 install virtualenv
	python3 -m virtualenv web/cgi/.virtualenv

$(TARGET): $(OBJ)
	$(CC) $(CPPFLAGS) $(OBJ) -o $(TARGET)

$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	$(RM) $(OBJ_DIR)

fclean: clean
	$(RM) $(TARGET)
	$(RM) $(TMPDIR)
	$(RM) $(wildcard *.chunk)

re: fclean all

.PHONY: all clean fclean re
