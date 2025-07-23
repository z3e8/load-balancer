#ifndef LOAD_BALANCER_H
#define LOAD_BALANCER_H

#include <string>
#include <thread>
#include <atomic>
#include "BackendPool.h"
#include "ConnectionPool.h"

class LoadBalancer {
public:
    LoadBalancer(int port);
    ~LoadBalancer();
    void run();
    void load_config(const std::string& filename);
    void stop();
    
private:
    int server_fd;
    int port;
    BackendPool pool;
    ConnectionPool conn_pool;
    std::string strategy;
    std::thread health_check_thread;
    std::atomic<bool> running;
    std::atomic<int> request_count;
    int health_check_interval;
    void handle_client(int client_fd, struct sockaddr_in client_addr);
    int connect_to_backend(Backend* backend);
    void health_check_loop();
    bool check_backend_health(Backend* backend);
};

#endif

