#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <ctime>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

struct RequestLine
{
    std::string method;
    std::string path;
    std::string version;
};

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

std::string current_timestamp()
{
    std::time_t now = std::time(nullptr);
    char time_buffer[32];
    if (std::strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now)) == 0)
    {
        return "unknown-time";
    }
    return std::string(time_buffer);
}

void log_request(const std::string &method, const std::string &path, const std::string &version, int status_code)
{
    std::ofstream log_file("logs/server.log", std::ios::app);
    if (!log_file.good())
    {
        return;
    }

    log_file << "[" << current_timestamp() << "] "
             << method << " "
             << path << " "
             << version << " -> "
             << status_code << "\n";
}

std::string get_reason_phrase(int status_code)
{
    switch (status_code)
    {
    case 200:
        return "OK";
    case 400:
        return "Bad Request";
    case 404:
        return "Not Found";
    case 405:
        return "Method Not Allowed";
    case 505:
        return "HTTP Version Not Supported";
    default:
        return "Internal Server Error";
    }
}

std::string build_response(int status_code, const std::string &content_type, const std::string &body)
{
    std::ostringstream response;
    response << "HTTP/1.1 " << status_code << " " << get_reason_phrase(status_code) << "\r\n";
    response << "Content-Type: " << content_type << "\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Connection: close\r\n";
    if (status_code == 405)
    {
        response << "Allow: GET\r\n";
    }
    response << "\r\n";
    response << body;
    return response.str();
}

bool parse_request_line(const std::string &request_line, RequestLine &parsed)
{
    std::istringstream iss(request_line);
    std::string extra;

    if (!(iss >> parsed.method >> parsed.path >> parsed.version))
    {
        return false;
    }

    if (iss >> extra)
    {
        return false;
    }

    if (parsed.path.empty() || parsed.path[0] != '/')
    {
        return false;
    }

    return true;
}

bool is_supported_http_version(const std::string &version)
{
    return version == "HTTP/1.1" || version == "HTTP/1.0";
}

std::string strip_query_and_fragment(const std::string &path)
{
    size_t query_pos = path.find('?');
    size_t fragment_pos = path.find('#');
    size_t cut_pos = std::string::npos;

    if (query_pos != std::string::npos && fragment_pos != std::string::npos)
    {
        cut_pos = std::min(query_pos, fragment_pos);
    }
    else if (query_pos != std::string::npos)
    {
        cut_pos = query_pos;
    }
    else if (fragment_pos != std::string::npos)
    {
        cut_pos = fragment_pos;
    }

    if (cut_pos == std::string::npos)
    {
        return path;
    }

    return path.substr(0, cut_pos);
}

bool is_safe_path(const std::string &path)
{
    if (path.find("..") != std::string::npos)
    {
        return false;
    }
    return path.find('\\') == std::string::npos;
}

std::string get_mime_type(const std::string &path)
{
    static const std::unordered_map<std::string, std::string> mime_types = {
        {".html", "text/html"},
        {".htm", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".json", "application/json"},
        {".txt", "text/plain"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".svg", "image/svg+xml"},
        {".ico", "image/x-icon"},
        {".pdf", "application/pdf"}};

    size_t dot_pos = path.find_last_of('.');
    if (dot_pos == std::string::npos)
    {
        return "application/octet-stream";
    }

    std::string ext = path.substr(dot_pos);
    auto it = mime_types.find(ext);
    if (it != mime_types.end())
    {
        return it->second;
    }

    return "application/octet-stream";
}

bool read_file_bytes(const std::string &path, std::string &contents)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.good())
    {
        return false;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    contents = ss.str();
    return true;
}

int main()
{
    mkdir("logs", 0755);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (server_fd < 0)
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
        int status_code = 0;
        std::string log_method = "-";
        std::string log_path = "-";
        std::string log_version = "-";
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

            if (request.size() > 16384)
            {
                std::string response = build_response(400, "text/plain", "400 Bad Request");
                send_all(client_socket, response);
                status_code = 400;
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
            std::string response = build_response(400, "text/plain", "400 Bad Request");
            send_all(client_socket, response);
            log_request(log_method, log_path, log_version, 400);
            close(client_socket);
            continue;
        }
        std::string request_line = request.substr(0, pos);

        std::cout << "Request Line: " << request_line << std::endl;

        RequestLine parsed_request{};
        if (!parse_request_line(request_line, parsed_request))
        {
            std::string response = build_response(400, "text/plain", "400 Bad Request");
            send_all(client_socket, response);
            log_request(log_method, log_path, log_version, 400);
            close(client_socket);
            continue;
        }

        log_method = parsed_request.method;
        log_path = parsed_request.path;
        log_version = parsed_request.version;

        std::cout << "Method: " << parsed_request.method << std::endl;
        std::cout << "Path: " << parsed_request.path << std::endl;
        std::cout << "Version: " << parsed_request.version << std::endl;

        if (!is_supported_http_version(parsed_request.version))
        {
            std::string response = build_response(505, "text/plain", "505 HTTP Version Not Supported");
            send_all(client_socket, response);
            log_request(log_method, log_path, log_version, 505);
            close(client_socket);
            continue;
        }

        if (parsed_request.method != "GET")
        {
            std::string response = build_response(405, "text/plain", "405 Method Not Allowed");
            send_all(client_socket, response);
            log_request(log_method, log_path, log_version, 405);
            close(client_socket);
            continue;
        }

        std::string path = strip_query_and_fragment(parsed_request.path);
        if (!is_safe_path(path))
        {
            std::string response = build_response(400, "text/plain", "400 Bad Request");
            send_all(client_socket, response);
            log_request(log_method, log_path, log_version, 400);
            close(client_socket);
            continue;
        }

        // If root path, serve index.html
        if (path == "/")
        {
            path = "/index.html";
        }

        std::string full_path = "www" + path;
        std::string file_content;

        if (read_file_bytes(full_path, file_content))
        {
            std::string response_str = build_response(200, get_mime_type(path), file_content);
            send_all(client_socket, response_str);
            status_code = 200;
        }
        else
        {
            std::string response = build_response(404, "text/plain", "404 Not Found");
            send_all(client_socket, response);
            status_code = 404;
        }
        log_request(log_method, log_path, log_version, status_code);
        close(client_socket);
    }

    return 0;
}
