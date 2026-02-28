#include <iostream>
#include <string>

#include "files.h"
#include "http.h"

using namespace std;

bool check(bool condition, const string &name)
{
    if (condition)
    {
        cout << "PASS: " << name << "\n";
        return true;
    }

    cerr << "FAIL: " << name << "\n";
    return false;
}

int main()
{
    bool all_passed = true;

    RequestLine valid{};
    all_passed &= check(parse_request_line("GET / HTTP/1.1", valid), "parse valid request line");
    all_passed &= check(valid.method == "GET", "method parsed correctly");
    all_passed &= check(valid.path == "/", "path parsed correctly");
    all_passed &= check(valid.version == "HTTP/1.1", "version parsed correctly");

    RequestLine malformed_missing_parts{};
    all_passed &= check(!parse_request_line("GET /", malformed_missing_parts), "reject missing version");

    RequestLine malformed_extra_token{};
    all_passed &= check(!parse_request_line("GET / HTTP/1.1 EXTRA", malformed_extra_token), "reject extra token");

    RequestLine malformed_path{};
    all_passed &= check(!parse_request_line("GET index.html HTTP/1.1", malformed_path), "reject non-slash path");

    all_passed &= check(strip_query_and_fragment("/index.html?lang=en#top") == "/index.html", "strip query and fragment");
    all_passed &= check(strip_query_and_fragment("/style.css") == "/style.css", "preserve plain path");

    all_passed &= check(is_safe_path("/index.html"), "safe path accepted");
    all_passed &= check(!is_safe_path("/../etc/passwd"), "reject traversal path");
    all_passed &= check(!is_safe_path("/foo\\bar"), "reject backslash path");

    if (!all_passed)
    {
        return 1;
    }

    cout << "All unit tests passed.\n";
    return 0;
}
