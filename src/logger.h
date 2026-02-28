#ifndef LOGGER_H
#define LOGGER_H

#include <string>

void log_request(const std::string &method, const std::string &path, const std::string &version, int status_code);
void log_incoming_request(const std::string &raw_request);
void log_parsed_request_line(const std::string &request_line);
void log_parsed_request_parts(const std::string &method, const std::string &path, const std::string &version);

#endif
