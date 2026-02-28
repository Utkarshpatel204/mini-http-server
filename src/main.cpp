#include <iostream>
#include <cstring>
#include <sstream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

bool send_all(int socket_fd, const std::string &data)
{
    size_t total_sent = 0;
    while (total_sent < data.size())
    {
        ssize_t sent = send(socket_fd, data.data() + total_sent, data.size() - total_sent, 0);
        if (sent <= 0)
        {
            return false;
        }
        total_sent += static_cast<size_t>(sent);
    }
    return true;
}

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (server_fd == 0)
    {
        perror("socket failed");
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        return 1;
    }

    if (listen(server_fd, 10) < 0)
    {
        perror("listen");
        return 1;
    }

    std::cout << "Server running on port 8080...\n";

    while (true)
    {
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0)
        {
            perror("accept");
            continue;
        }

        std::string request;
        char buffer[4096];
        while (true)
        {
            ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
            if (bytes_read <= 0)
            {
                break;
            }

            request.append(buffer, bytes_read);

            if (request.find("\r\n\r\n") != std::string::npos)
            {
                break;
            }
        }

        if (request.empty())
        {
            close(client_socket);
            continue;
        }

        std::cout << "----- Incoming Request -----\n";
        std::cout << request << "\n";
        std::cout << "----------------------------\n";

        // Find first line (request line)
        size_t pos = request.find("\r\n");
        if (pos == std::string::npos)
        {
            close(client_socket);
            continue;
        }
        std::string request_line = request.substr(0, pos);

        std::cout << "Request Line: " << request_line << std::endl;

        // Split request line into parts
        std::istringstream iss(request_line);
        std::string method, path, version;
        iss >> method >> path >> version;

        std::cout << "Method: " << method << std::endl;
        std::cout << "Path: " << path << std::endl;
        std::cout << "Version: " << version << std::endl;

        // If root path, serve index.html
        if (path == "/")
        {
            path = "/index.html";
        }

        std::string full_path = "www" + path;

        // Try opening the file
        std::ifstream file(full_path, std::ios::binary);

        if (file.good())
        {
            std::ostringstream ss;
            ss << file.rdbuf();
            std::string file_content = ss.str();

            std::ostringstream response;
            response << "HTTP/1.1 200 OK\r\n";
            response << "Content-Type: text/html\r\n";
            response << "Content-Length: " << file_content.size() << "\r\n";
            response << "\r\n";
            response << file_content;

            std::string response_str = response.str();
            send_all(client_socket, response_str);
        }
        else
        {
            std::string not_found =
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 13\r\n"
                "\r\n"
                "404 Not Found";

            send_all(client_socket, not_found);
        }
        close(client_socket);
    }

    return 0;
}
