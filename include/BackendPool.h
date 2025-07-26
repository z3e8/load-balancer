#ifndef BACKEND_POOL_H
#define BACKEND_POOL_H

#include <string>
#include <vector>
#include <mutex>

struct Backend {
    std::string host;
    int port;
    int active_connections = 0;
    bool is_healthy = true;
};

// Manages backend servers and selection algorithms
// Round-robin: cycles through backends in order
// Least-connections: picks backend with fewest active connections
class BackendPool {
public:
    void add_backend(const std::string& host, int port);
    Backend* select_round_robin();
    Backend* select_least_connections();
    Backend* select(const std::string& strategy);
    std::vector<Backend>& get_backends() { return backends; }
    void update_health(const std::string& host, int port, bool healthy);
    void increment_connections(const std::string& host, int port);
    void decrement_connections(const std::string& host, int port);
    
private:
    std::vector<Backend> backends;
    int round_robin_counter = 0;
    std::mutex mutex;
};

#endif

