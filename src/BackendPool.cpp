#include "../include/BackendPool.h"
#include <mutex>

void BackendPool::add_backend(const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(mutex);
    backends.push_back({host, port});
}

Backend* BackendPool::select_round_robin() {
    std::lock_guard<std::mutex> lock(mutex);
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
    std::lock_guard<std::mutex> lock(mutex);
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

void BackendPool::update_health(const std::string& host, int port, bool healthy) {
    std::lock_guard<std::mutex> lock(mutex);
    for (auto& backend : backends) {
        if (backend.host == host && backend.port == port) {
            backend.is_healthy = healthy;
            break;
        }
    }
}

void BackendPool::increment_connections(const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(mutex);
    for (auto& backend : backends) {
        if (backend.host == host && backend.port == port) {
            backend.active_connections++;
            break;
        }
    }
}

void BackendPool::decrement_connections(const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(mutex);
    for (auto& backend : backends) {
        if (backend.host == host && backend.port == port) {
            backend.active_connections--;
            break;
        }
    }
}

