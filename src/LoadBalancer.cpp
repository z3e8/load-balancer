#include "../include/LoadBalancer.h"
#include "../include/HttpParser.h"
#include "../include/Logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/time.h>
#include <chrono>
#include <thread>
#include <cerrno>
#include <csignal>
#include <atomic>

static LoadBalancer* g_load_balancer = nullptr;

void signal_handler(int sig) {
    if (g_load_balancer) {
        g_load_balancer->stop();
    }
}

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

std::string extract_string(const std::string& line) {
    size_t start = line.find('"');
    if (start == std::string::npos) return "";
    size_t end = line.find('"', start + 1);
    if (end == std::string::npos) return "";
    return line.substr(start + 1, end - start - 1);
}

int extract_int(const std::string& line) {
    size_t start = line.find(':');
    if (start == std::string::npos) return 0;
    std::string num_str = trim(line.substr(start + 1));
    num_str.erase(std::remove(num_str.begin(), num_str.end(), ','), num_str.end());
    return std::stoi(num_str);
}

LoadBalancer::LoadBalancer(int port) : port(port), server_fd(-1), running(false), request_count(0), health_check_interval(10) {
}

LoadBalancer::~LoadBalancer() {
    stop();
    if (server_fd >= 0) {
        close(server_fd);
    }
    conn_pool.close_all();
}

void LoadBalancer::stop() {
    running = false;
    if (health_check_thread.joinable()) {
        health_check_thread.join();
    }
}

void LoadBalancer::load_config(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file, using defaults" << std::endl;
        pool.add_backend("localhost", 8001);
        pool.add_backend("localhost", 8002);
        pool.add_backend("localhost", 8003);
        return;
    }

    std::string line;
    bool in_backends = false;
    bool in_backend_obj = false;
    std::string current_host;
    int current_port = 0;

    while (std::getline(file, line)) {
        line = trim(line);
        if (line.find("\"backends\"") != std::string::npos) {
            in_backends = true;
            continue;
        }
        if (line.find("\"strategy\"") != std::string::npos) {
            strategy = extract_string(line);
            continue;
        }
        if (in_backends) {
            if (line == "[") continue;
            if (line == "{") {
                in_backend_obj = true;
                current_host = "";
                current_port = 0;
                continue;
            }
            if (line == "}" || line == "},") {
                if (!current_host.empty() && current_port > 0) {
                    pool.add_backend(current_host, current_port);
                }
                in_backend_obj = false;
                continue;
            }
            if (in_backend_obj) {
                if (line.find("\"host\"") != std::string::npos) {
                    current_host = extract_string(line);
                } else if (line.find("\"port\"") != std::string::npos) {
                    current_port = extract_int(line);
                }
            }
        }
    }

    if (pool.get_backends().empty()) {
        pool.add_backend("localhost", 8001);
        pool.add_backend("localhost", 8002);
        pool.add_backend("localhost", 8003);
    }
}

int LoadBalancer::connect_to_backend(Backend* backend) {
    int backend_fd = conn_pool.get_connection(backend->host, backend->port);
    if (backend_fd >= 0) {
        return backend_fd;
    }

    backend_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (backend_fd < 0) {
        std::cerr << "Error creating socket for " << backend->host << ":" << backend->port 
                  << " (errno: " << errno << "): ";
        perror("");
        return -1;
    }

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(backend_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(backend_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    struct hostent *server = gethostbyname(backend->host.c_str());
    if (server == nullptr) {
        std::cerr << "Error resolving host " << backend->host << " (errno: " << h_errno << "): ";
        herror("");
        close(backend_fd);
        return -1;
    }

    struct sockaddr_in backend_addr;
    backend_addr.sin_family = AF_INET;
    backend_addr.sin_port = htons(backend->port);
    memcpy(&backend_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(backend_fd, (struct sockaddr *)&backend_addr, sizeof(backend_addr)) < 0) {
        if (errno == ETIMEDOUT || errno == ECONNREFUSED) {
            std::cerr << "Connection to " << backend->host << ":" << backend->port 
                      << " failed: " << (errno == ETIMEDOUT ? "timeout" : "connection refused") 
                      << " (errno: " << errno << ")" << std::endl;
        } else {
            std::cerr << "Error connecting to " << backend->host << ":" << backend->port 
                      << " (errno: " << errno << "): ";
            perror("");
        }
        close(backend_fd);
        return -1;
    }

    return backend_fd;
}

void LoadBalancer::handle_client(int client_fd, struct sockaddr_in client_addr) {
    char buffer[4096] = {0};
    int bytes_read = read(client_fd, buffer, 4096);
    if (bytes_read <= 0) {
        close(client_fd);
        return;
    }

    request_count++;
    std::string request_str(buffer);
    HttpRequest req = HttpParser::parse(request_str);

    std::vector<Backend>& backends = pool.get_backends();
    Backend* selected = nullptr;
    int backend_fd = -1;

    for (size_t i = 0; i < backends.size(); i++) {
        selected = pool.select(strategy);
        if (selected == nullptr) break;

        backend_fd = connect_to_backend(selected);
        if (backend_fd >= 0) {
            pool.increment_connections(selected->host, selected->port);
            break;
        }
        std::cerr << "Failed to connect to backend " << selected->host << ":" << selected->port << std::endl;
    }

    if (backend_fd < 0) {
        std::cerr << "All backends failed for request from " << inet_ntoa(client_addr.sin_addr) 
                  << ", closing client connection" << std::endl;
        close(client_fd);
        return;
    }

    Logger::log_request(inet_ntoa(client_addr.sin_addr), req.method, req.path,
                       selected->host + ":" + std::to_string(selected->port), request_count.load());

    send(backend_fd, buffer, bytes_read, 0);

    char response_buffer[4096] = {0};
    int response_bytes = read(backend_fd, response_buffer, 4096);
    while (response_bytes > 0) {
        send(client_fd, response_buffer, response_bytes, 0);
        response_bytes = read(backend_fd, response_buffer, 4096);
    }

    conn_pool.return_connection(selected->host, selected->port, backend_fd);
    pool.decrement_connections(selected->host, selected->port);
    close(client_fd);
}

void LoadBalancer::run() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Error creating server socket (errno: " << errno << "): ";
        perror("");
        return;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Error binding to port " << port << " (errno: " << errno << "): ";
        perror("");
        return;
    }

    if (listen(server_fd, 10) < 0) {
        std::cerr << "Error listening on socket (errno: " << errno << "): ";
        perror("");
        return;
    }

    std::cout << "Server listening on port " << port << std::endl;
    std::cout << "Strategy: " << strategy << std::endl;
    std::cout << "Backends: ";
    for (const auto& b : pool.get_backends()) {
        std::cout << b.host << ":" << b.port << " ";
    }
    std::cout << std::endl;

    g_load_balancer = this;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    running = true;
    health_check_thread = std::thread(&LoadBalancer::health_check_loop, this);

    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        
        if (client_fd < 0) {
            if (!running) break;
            continue;
        }

        handle_client(client_fd, client_addr);
    }

    std::cout << "Shutting down gracefully..." << std::endl;
    std::cout << "Total requests processed: " << request_count.load() << std::endl;
    stop();
    if (server_fd >= 0) {
        close(server_fd);
    }
}

bool LoadBalancer::check_backend_health(Backend* backend) {
    int test_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (test_fd < 0) {
        std::cerr << "Health check: socket creation failed for " << backend->host << ":" << backend->port 
                  << " (errno: " << errno << ")" << std::endl;
        return false;
    }

    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    setsockopt(test_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(test_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    struct hostent *server = gethostbyname(backend->host.c_str());
    if (server == nullptr) {
        std::cerr << "Health check: host resolution failed for " << backend->host 
                  << " (errno: " << h_errno << ")" << std::endl;
        close(test_fd);
        return false;
    }

    struct sockaddr_in backend_addr;
    backend_addr.sin_family = AF_INET;
    backend_addr.sin_port = htons(backend->port);
    memcpy(&backend_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    bool healthy = connect(test_fd, (struct sockaddr *)&backend_addr, sizeof(backend_addr)) >= 0;
    if (!healthy) {
        if (errno == ETIMEDOUT) {
            std::cerr << "Health check: " << backend->host << ":" << backend->port << " timeout" << std::endl;
        } else if (errno == ECONNREFUSED) {
            std::cerr << "Health check: " << backend->host << ":" << backend->port << " connection refused" << std::endl;
        } else {
            std::cerr << "Health check: " << backend->host << ":" << backend->port 
                      << " failed (errno: " << errno << ")" << std::endl;
        }
    }
    close(test_fd);
    return healthy;
}

void LoadBalancer::health_check_loop() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(health_check_interval));
        
        if (!running) break;
        
        for (auto& backend : pool.get_backends()) {
            bool healthy = check_backend_health(&backend);
            pool.update_health(backend.host, backend.port, healthy);
        }
    }
}

