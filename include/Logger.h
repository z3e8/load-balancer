#ifndef LOGGER_H
#define LOGGER_H

#include <string>

class Logger {
public:
    static void log_request(const std::string& client_ip, const std::string& method, 
                           const std::string& path, const std::string& backend, int request_num);
};

#endif

