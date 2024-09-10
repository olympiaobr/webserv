# Webserv

Webserv is a lightweight HTTP server built in C++, designed to comply with HTTP/1.1 standards. The goal of this project, as part of the 42 curriculum, is to help students understand the fundamentals of web server architecture, network programming, and handling HTTP requests/responses without relying on external libraries.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Installation](#installation)
- [Usage](#usage)
- [Configuration](#configuration)
- [Project Structure](#project-structure)

## Overview

The `webserv` project is a custom implementation of a web server, created using C++ with a focus on handling HTTP/1.1 requests. The project demonstrates the basics of socket programming, multiplexing using `select`, and handling multiple concurrent connections, as well as implementing various HTTP methods.

## Features

- Supports HTTP/1.1 protocol
- Handles multiple client connections simultaneously
- Supports HTTP methods: `GET`, `POST`, `DELETE`
- Serves static files
- CGI support for dynamic content
- Custom error pages
- Configurable via configuration file
- Lightweight and minimal dependencies

## Installation

To get started with `webserv`, follow these instructions:

1. Clone the repository:
    ```bash
    git clone https://github.com/ddavlet/webserv.git
    cd webserv
    ```

2. Compile the server:
    ```bash
    make
    ```

This will generate the `webserv` executable in the root directory.

## Usage

Run the server with the following command:
```bash
./webserv path/to/config_file.conf
```

You can use a custom configuration file to define the server’s behavior (explained in the Configuration section).

Example:
```bash
./webserv ./config/webserv.conf
```
After starting the server, you can access it via a browser by navigating to http://localhost:<port>.

### Configuration

The server can be configured using a .conf file that defines various settings such as:

Listening ports
Server names
Root directories
Index files
Allowed methods (GET, POST, DELETE)
Error pages
Client body size limits
CGI support
Example configuration:

```conf
server {
    listen 8080;
    server_name localhost;

    location / {
        root /var/www/html;
        index index.html;
    }

    location /cgi-bin/ {
        cgi_pass /usr/bin/php-cgi;
    }

    error_page 404 /errors/404.html;
    client_max_body_size 2M;
}
```
## Project Structure

/src: Contains all the source files for the server.
/includes: Contains header files for the server.
/config: Contains example configuration files.
Makefile: Build script for compiling the project.
