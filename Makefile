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
			cgi/CGI.cpp \
			server/Session.cpp

TMPDIR = tmp

CGI_ENV = web/cgi/.venv

SRC := $(addprefix $(SRC_DIR), $(SOURCE))
OBJ := $(addprefix $(OBJ_DIR), $(SOURCE:.cpp=.o))

all: $(TARGET) $(TMPDIR) $(CGI_ENV)

$(TMPDIR):
	mkdir -p $(TMPDIR)

$(CGI_ENV):
	@if python3 -m venv $(CGI_ENV) 2>/dev/null; then \
		echo "Virtual environment created using venv"; \
	else \
		echo "venv failed, trying virtualenv"; \
		/usr/local/bin/virtualenv $(CGI_ENV); \
	fi
	$(CGI_ENV)/bin/python -m pip install --upgrade pip setuptools wheel
	$(CGI_ENV)/bin/pip install --use-pep517 -r web/cgi/requirements.txt

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
	$(RM) $(CGI_ENV)

re: fclean all

.PHONY: all clean fclean re
