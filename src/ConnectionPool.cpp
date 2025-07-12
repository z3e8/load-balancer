#include "../include/ConnectionPool.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <sstream>

std::string ConnectionPool::make_key(const std::string& host, int port) {
    return host + ":" + std::to_string(port);
}

int ConnectionPool::get_connection(const std::string& host, int port) {
    std::string key = make_key(host, port);
    
    if (pool.find(key) != pool.end() && !pool[key].empty()) {
        int fd = pool[key].back();
        pool[key].pop_back();
        return fd;
    }
    
    return -1;
}

void ConnectionPool::return_connection(const std::string& host, int port, int fd) {
    std::string key = make_key(host, port);
    pool[key].push_back(fd);
}

void ConnectionPool::close_all() {
    for (auto& pair : pool) {
        for (int fd : pair.second) {
            close(fd);
        }
        pair.second.clear();
    }
    pool.clear();
}

