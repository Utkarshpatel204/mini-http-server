#ifndef HTTP_H
#define HTTP_H

#include <string>

struct RequestLine
{
    std::string method;
    std::string path;
    std::string version;
};

std::string build_response(int status_code, const std::string &content_type, const std::string &body);
bool parse_request_line(const std::string &request_line, RequestLine &parsed);
bool is_supported_http_version(const std::string &version);

#endif
