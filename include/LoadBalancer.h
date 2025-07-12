#ifndef LOAD_BALANCER_H
#define LOAD_BALANCER_H

#include <string>
#include "BackendPool.h"
#include "ConnectionPool.h"

class LoadBalancer {
public:
    LoadBalancer(int port);
    ~LoadBalancer();
    void run();
    void load_config(const std::string& filename);
    
private:
    int server_fd;
    int port;
    BackendPool pool;
    ConnectionPool conn_pool;
    std::string strategy;
    void handle_client(int client_fd, struct sockaddr_in client_addr);
    int connect_to_backend(Backend* backend);
};

#endif

