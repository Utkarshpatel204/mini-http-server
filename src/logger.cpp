#include "logger.h"

#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>

using namespace std;

namespace
{
mutex g_log_mutex;
mutex g_console_mutex;

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
} // namespace

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

void log_incoming_request(const string &raw_request)
{
    lock_guard<mutex> lock(g_console_mutex);
    cout << "----- Incoming Request -----\n";
    cout << raw_request << "\n";
    cout << "----------------------------\n";
}

void log_parsed_request_line(const string &request_line)
{
    lock_guard<mutex> lock(g_console_mutex);
    cout << "Request Line: " << request_line << endl;
}

void log_parsed_request_parts(const string &method, const string &path, const string &version)
{
    lock_guard<mutex> lock(g_console_mutex);
    cout << "Method: " << method << endl;
    cout << "Path: " << path << endl;
    cout << "Version: " << version << endl;
}
