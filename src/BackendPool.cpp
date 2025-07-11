#include "../include/BackendPool.h"

void BackendPool::add_backend(const std::string& host, int port) {
    backends.push_back({host, port});
}

Backend* BackendPool::select_round_robin() {
    if (backends.empty()) return nullptr;
    int idx = round_robin_counter % backends.size();
    round_robin_counter++;
    return &backends[idx];
}

Backend* BackendPool::select_least_connections() {
    if (backends.empty()) return nullptr;
    int least_idx = 0;
    for (size_t i = 1; i < backends.size(); i++) {
        if (backends[i].active_connections < backends[least_idx].active_connections) {
            least_idx = i;
        }
    }
    return &backends[least_idx];
}

Backend* BackendPool::select(const std::string& strategy) {
    if (strategy == "least_connections") {
        return select_least_connections();
    }
    return select_round_robin();
}

