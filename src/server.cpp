#include "server.h"

#include "files.h"
#include "http.h"
#include "logger.h"

#include <atomic>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

namespace
{
atomic<int> g_active_clients{0};

bool send_all(int socket_fd, const string &data)
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

void handle_client(int client_socket)
{
    string request;
    int status_code = 0;
    bool request_too_large = false;
    string log_method = "-";
    string log_path = "-";
    string log_version = "-";
    char buffer[4096];

    while (true)
    {
        ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_read <= 0)
        {
            break;
        }

        request.append(buffer, bytes_read);

        if (request.find("\r\n\r\n") != string::npos)
        {
            break;
        }

        if (request.size() > 16384)
        {
            string response = build_response(400, "text/plain", "400 Bad Request");
            send_all(client_socket, response);
            log_request(log_method, log_path, log_version, 400);
            request_too_large = true;
            break;
        }
    }

    if (request.empty() || request_too_large)
    {
        close(client_socket);
        return;
    }

    log_incoming_request(request);

    size_t pos = request.find("\r\n");
    if (pos == string::npos)
    {
        string response = build_response(400, "text/plain", "400 Bad Request");
        send_all(client_socket, response);
        log_request(log_method, log_path, log_version, 400);
        close(client_socket);
        return;
    }

    string request_line = request.substr(0, pos);
    log_parsed_request_line(request_line);

    RequestLine parsed_request{};
    if (!parse_request_line(request_line, parsed_request))
    {
        string response = build_response(400, "text/plain", "400 Bad Request");
        send_all(client_socket, response);
        log_request(log_method, log_path, log_version, 400);
        close(client_socket);
        return;
    }

    log_method = parsed_request.method;
    log_path = parsed_request.path;
    log_version = parsed_request.version;
    log_parsed_request_parts(parsed_request.method, parsed_request.path, parsed_request.version);

    if (!is_supported_http_version(parsed_request.version))
    {
        string response = build_response(505, "text/plain", "505 HTTP Version Not Supported");
        send_all(client_socket, response);
        log_request(log_method, log_path, log_version, 505);
        close(client_socket);
        return;
    }

    if (parsed_request.method != "GET")
    {
        string response = build_response(405, "text/plain", "405 Method Not Allowed");
        send_all(client_socket, response);
        log_request(log_method, log_path, log_version, 405);
        close(client_socket);
        return;
    }

    string path = strip_query_and_fragment(parsed_request.path);
    if (!is_safe_path(path))
    {
        string response = build_response(400, "text/plain", "400 Bad Request");
        send_all(client_socket, response);
        log_request(log_method, log_path, log_version, 400);
        close(client_socket);
        return;
    }

    if (path == "/")
    {
        path = "/index.html";
    }

    string full_path = "www" + path;
    string file_content;

    if (read_file_bytes(full_path, file_content))
    {
        string response_str = build_response(200, get_mime_type(path), file_content);
        send_all(client_socket, response_str);
        status_code = 200;
    }
    else
    {
        string response = build_response(404, "text/plain", "404 Not Found");
        send_all(client_socket, response);
        status_code = 404;
    }

    log_request(log_method, log_path, log_version, status_code);
    close(client_socket);
}

void handle_client_and_release(int client_socket)
{
    handle_client(client_socket);
    g_active_clients.fetch_sub(1);
}
} // namespace

int run_server(int port, int max_concurrent_clients)
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
    address.sin_port = htons(port);

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

    cout << "Server running on port " << port << "...\n";
    cout << "Max concurrent clients: " << max_concurrent_clients << "\n";

    while (true)
    {
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0)
        {
            perror("accept");
            continue;
        }

        int active_clients = g_active_clients.load();
        if (active_clients >= max_concurrent_clients)
        {
            string response = build_response(503, "text/plain", "503 Service Unavailable");
            send_all(client_socket, response);
            log_request("-", "-", "-", 503);
            close(client_socket);
            continue;
        }

        g_active_clients.fetch_add(1);
        thread client_thread(handle_client_and_release, client_socket);
        client_thread.detach();
    }

    return 0;
}
