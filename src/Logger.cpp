#include "../include/Logger.h"
#include <iostream>
#include <ctime>
#include <iomanip>

void Logger::log_request(const std::string& client_ip, const std::string& method,
                         const std::string& path, const std::string& backend) {
    std::time_t now = std::time(nullptr);
    std::tm* timeinfo = std::localtime(&now);
    std::cout << std::put_time(timeinfo, "%Y-%m-%d %H:%M:%S") << " "
              << client_ip << " "
              << method << " " << path << " -> "
              << backend << std::endl;
}

