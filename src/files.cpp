#include "files.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>

using namespace std;

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
