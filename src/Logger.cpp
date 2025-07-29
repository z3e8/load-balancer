#include "../include/Logger.h"
#include <iostream>
#include <ctime>
#include <iomanip>

std::chrono::steady_clock::time_point Logger::start_time = std::chrono::steady_clock::now();
std::atomic<int> Logger::total_requests(0);
std::atomic<double> Logger::total_latency_ms(0.0);

void Logger::log_request(const std::string& client_ip, const std::string& method,
                         const std::string& path, const std::string& backend, int request_num,
                         double latency_ms) {
    std::time_t now = std::time(nullptr);
    std::tm* timeinfo = std::localtime(&now);
    
    total_requests++;
    total_latency_ms += latency_ms;
    
    auto elapsed = std::chrono::steady_clock::now() - start_time;
    double elapsed_sec = std::chrono::duration<double>(elapsed).count();
    double req_per_sec = elapsed_sec > 0 ? total_requests.load() / elapsed_sec : 0.0;
    
    std::cout << std::put_time(timeinfo, "%Y-%m-%d %H:%M:%S") << " "
              << "[" << request_num << "] "
              << client_ip << " "
              << method << " " << path << " -> "
              << backend << " "
              << std::fixed << std::setprecision(2) << latency_ms << "ms" << std::endl;
    
    // log metrics every 10 requests
    if (total_requests.load() % 10 == 0) {
        double avg_latency = total_requests.load() > 0 ? total_latency_ms.load() / total_requests.load() : 0.0;
        log_metrics(req_per_sec, avg_latency);
    }
}

void Logger::log_metrics(double requests_per_sec, double avg_latency_ms) {
    std::cout << "[METRICS] " << std::fixed << std::setprecision(2) 
              << requests_per_sec << " req/s, avg latency: " 
              << avg_latency_ms << "ms" << std::endl;
}

