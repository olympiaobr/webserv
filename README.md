# Webserv

## 🌐 Overview
This project is a C++ implementation of a **web server**, developed as part of the 42 curriculum. The goal of the project is to create a fully functional web server, handling HTTP requests and serving content in compliance with the HTTP/1.1 protocol.

## 💡 Features
- **GET, POST, DELETE** request handling
- Supports **multiple clients** simultaneously
- Configurable through a custom `.conf` file
- Handles static files and CGI execution

## 🛠️ Getting Started
1. Clone the repository:
   ```bash
   git clone https://github.com/ddavlet/webserv.git
   ```
Navigate to the project directory:
  ```bash
  cd webserv
  ```
Compile the server:
  ```bash
  make
  ```
Run the server:
  ```bash
  ./webserv c_file.conf
  ```
## 🧪 Testing
Tests are located in the /tests directory. Run them with:
  ```bash
  ./run_tests.sh
  ```
## 📜 License
This project is licensed under the MIT License.
