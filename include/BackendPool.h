#ifndef BACKEND_POOL_H
#define BACKEND_POOL_H

#include <string>
#include <vector>

struct Backend {
    std::string host;
    int port;
    int active_connections = 0;
    bool is_healthy = true;
};

class BackendPool {
public:
    void add_backend(const std::string& host, int port);
    Backend* select_round_robin();
    Backend* select_least_connections();
    Backend* select(const std::string& strategy);
    std::vector<Backend>& get_backends() { return backends; }
    
private:
    std::vector<Backend> backends;
    int round_robin_counter = 0;
};

#endif

