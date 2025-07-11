#include "../include/HttpParser.h"
#include <sstream>

HttpRequest HttpParser::parse(const std::string& request_str) {
    HttpRequest req;
    std::istringstream iss(request_str);
    std::string first_line;
    std::getline(iss, first_line);
    
    std::istringstream first_line_stream(first_line);
    first_line_stream >> req.method >> req.path;
    
    std::string header_line;
    while (std::getline(iss, header_line) && header_line != "\r" && !header_line.empty()) {
        size_t colon_pos = header_line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = header_line.substr(0, colon_pos);
            std::string value = header_line.substr(colon_pos + 1);
            while (!value.empty() && (value[0] == ' ' || value[0] == '\r')) {
                value.erase(0, 1);
            }
            req.headers[key] = value;
        }
    }
    
    return req;
}

