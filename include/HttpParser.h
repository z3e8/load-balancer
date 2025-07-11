#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <string>
#include <unordered_map>

struct HttpRequest {
    std::string method;
    std::string path;
    std::unordered_map<std::string, std::string> headers;
};

class HttpParser {
public:
    static HttpRequest parse(const std::string& request_str);
};

#endif

