#ifndef FILES_H
#define FILES_H

#include <string>

std::string strip_query_and_fragment(const std::string &path);
bool is_safe_path(const std::string &path);
std::string get_mime_type(const std::string &path);
bool read_file_bytes(const std::string &path, std::string &contents);

#endif
