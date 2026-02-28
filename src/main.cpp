#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <ctime>
#include <thread>
#include <mutex>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

struct RequestLine
{
    string method;
    string path;
    string version;
};

mutex g_log_mutex;
mutex g_console_mutex;

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

string current_timestamp()
{
    time_t now = time(nullptr);
    tm local_tm{};
    localtime_r(&now, &local_tm);
    char time_buffer[32];
    if (strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", &local_tm) == 0)
    {
        return "unknown-time";
    }
    return string(time_buffer);
}

void log_request(const string &method, const string &path, const string &version, int status_code)
{
    lock_guard<mutex> lock(g_log_mutex);
    ofstream log_file("logs/server.log", ios::app);
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

string get_reason_phrase(int status_code)
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

string build_response(int status_code, const string &content_type, const string &body)
{
    ostringstream response;
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

bool parse_request_line(const string &request_line, RequestLine &parsed)
{
    istringstream iss(request_line);
    string extra;

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

bool is_supported_http_version(const string &version)
{
    return version == "HTTP/1.1" || version == "HTTP/1.0";
}

string strip_query_and_fragment(const string &path)
{
    size_t query_pos = path.find('?');
    size_t fragment_pos = path.find('#');
    size_t cut_pos = string::npos;

    if (query_pos != string::npos && fragment_pos != string::npos)
    {
        cut_pos = min(query_pos, fragment_pos);
    }
    else if (query_pos != string::npos)
    {
        cut_pos = query_pos;
    }
    else if (fragment_pos != string::npos)
    {
        cut_pos = fragment_pos;
    }

    if (cut_pos == string::npos)
    {
        return path;
    }

    return path.substr(0, cut_pos);
}

bool is_safe_path(const string &path)
{
    if (path.find("..") != string::npos)
    {
        return false;
    }
    return path.find('\\') == string::npos;
}

string get_mime_type(const string &path)
{
    static const unordered_map<string, string> mime_types = {
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
    if (dot_pos == string::npos)
    {
        return "application/octet-stream";
    }

    string ext = path.substr(dot_pos);
    auto it = mime_types.find(ext);
    if (it != mime_types.end())
    {
        return it->second;
    }

    return "application/octet-stream";
}

bool read_file_bytes(const string &path, string &contents)
{
    ifstream file(path, ios::binary);
    if (!file.good())
    {
        return false;
    }

    ostringstream ss;
    ss << file.rdbuf();
    contents = ss.str();
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

    {
        lock_guard<mutex> lock(g_console_mutex);
        cout << "----- Incoming Request -----\n";
        cout << request << "\n";
        cout << "----------------------------\n";
    }

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
    {
        lock_guard<mutex> lock(g_console_mutex);
        cout << "Request Line: " << request_line << endl;
    }

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

    {
        lock_guard<mutex> lock(g_console_mutex);
        cout << "Method: " << parsed_request.method << endl;
        cout << "Path: " << parsed_request.path << endl;
        cout << "Version: " << parsed_request.version << endl;
    }

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

    cout << "Server running on port 8080...\n";

    while (true)
    {
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0)
        {
            perror("accept");
            continue;
        }
        thread client_thread(handle_client, client_socket);
        client_thread.detach();
    }

    return 0;
}
