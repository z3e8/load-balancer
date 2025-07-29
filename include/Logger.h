#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <chrono>
#include <atomic>

class Logger {
public:
    static void log_request(const std::string& client_ip, const std::string& method, 
                           const std::string& path, const std::string& backend, int request_num,
                           double latency_ms);
    static void log_metrics(double requests_per_sec, double avg_latency_ms);
    
private:
    static std::chrono::steady_clock::time_point start_time;
    static std::atomic<int> total_requests;
    static std::atomic<double> total_latency_ms;
};

#endif

