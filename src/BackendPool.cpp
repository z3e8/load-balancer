#include "../include/BackendPool.h"

void BackendPool::add_backend(const std::string& host, int port) {
    backends.push_back({host, port});
}

Backend* BackendPool::select_round_robin() {
    if (backends.empty()) return nullptr;
    
    int start_idx = round_robin_counter % backends.size();
    for (size_t i = 0; i < backends.size(); i++) {
        int idx = (start_idx + i) % backends.size();
        if (backends[idx].is_healthy) {
            round_robin_counter++;
            return &backends[idx];
        }
    }
    return nullptr;
}

Backend* BackendPool::select_least_connections() {
    if (backends.empty()) return nullptr;
    
    int least_idx = -1;
    for (size_t i = 0; i < backends.size(); i++) {
        if (!backends[i].is_healthy) continue;
        if (least_idx == -1 || backends[i].active_connections < backends[least_idx].active_connections) {
            least_idx = i;
        }
    }
    if (least_idx == -1) return nullptr;
    return &backends[least_idx];
}

Backend* BackendPool::select(const std::string& strategy) {
    if (strategy == "least_connections") {
        return select_least_connections();
    }
    return select_round_robin();
}

