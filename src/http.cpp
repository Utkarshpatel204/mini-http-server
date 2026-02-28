#include "http.h"

#include <sstream>

using namespace std;

static string get_reason_phrase(int status_code)
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
    case 503:
        return "Service Unavailable";
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
