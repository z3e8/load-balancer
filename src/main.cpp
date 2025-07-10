#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <netdb.h>
#include <ctime>
#include <iomanip>
#include <fstream>
#include <algorithm>

struct HttpRequest {
    std::string method;
    std::string path;
    std::unordered_map<std::string, std::string> headers;
};

struct Backend {
    std::string host;
    int port;
    int active_connections = 0;
};

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

void load_config(const std::string& filename, std::vector<Backend>& backends, std::string& strategy) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file, using defaults" << std::endl;
        return;
    }

    std::string line;
    bool in_backends = false;
    bool in_backend_obj = false;
    Backend current_backend;

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
                current_backend = Backend();
                continue;
            }
            if (line == "}" || line == "},") {
                backends.push_back(current_backend);
                in_backend_obj = false;
                continue;
            }
            if (in_backend_obj) {
                if (line.find("\"host\"") != std::string::npos) {
                    current_backend.host = extract_string(line);
                } else if (line.find("\"port\"") != std::string::npos) {
                    current_backend.port = extract_int(line);
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    std::string config_file = "config.json";
    if (argc > 1) {
        config_file = argv[1];
    }
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "socket failed" << std::endl;
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "bind failed" << std::endl;
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        std::cerr << "listen failed" << std::endl;
        return 1;
    }

    std::vector<Backend> backends;
    std::string strategy = "round_robin";
    load_config(config_file, backends, strategy);

    if (backends.empty()) {
        backends.push_back({"localhost", 8001});
        backends.push_back({"localhost", 8002});
        backends.push_back({"localhost", 8003});
    }

    std::cout << "Server listening on port 8080" << std::endl;
    std::cout << "Strategy: " << strategy << std::endl;
    std::cout << "Backends: ";
    for (const auto& b : backends) {
        std::cout << b.host << ":" << b.port << " ";
    }
    std::cout << std::endl;

    int round_robin_counter = 0;

    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        
        if (client_fd < 0) {
            continue;
        }

        char buffer[4096] = {0};
        int bytes_read = read(client_fd, buffer, 4096);
        if (bytes_read > 0) {
            std::string request_str(buffer);
            std::istringstream iss(request_str);
            std::string first_line;
            std::getline(iss, first_line);
            
            HttpRequest req;
            std::istringstream first_line_stream(first_line);
            first_line_stream >> req.method >> req.path;
            
            std::string header_line;
            while (std::getline(iss, header_line) && header_line != "\r" && !header_line.empty()) {
                size_t colon_pos = header_line.find(':');
                if (colon_pos != std::string::npos) {
                    std::string key = header_line.substr(0, colon_pos);
                    std::string value = header_line.substr(colon_pos + 1);
                    while (!value.empty() && (value[0] == ' ' || value[0] == '\r')) {
                        value.erase(0, 1);
                    }
                    req.headers[key] = value;
                }
            }

            int least_conn_idx = 0;
            for (size_t i = 1; i < backends.size(); i++) {
                if (backends[i].active_connections < backends[least_conn_idx].active_connections) {
                    least_conn_idx = i;
                }
            }

            int start_idx = least_conn_idx;
            int backend_fd = -1;
            Backend selected;
            bool connected = false;

            for (int i = 0; i < backends.size(); i++) {
                int idx = (start_idx + i) % backends.size();
                selected = backends[idx];

                std::time_t now = std::time(nullptr);
                std::tm* timeinfo = std::localtime(&now);
                std::cout << std::put_time(timeinfo, "%Y-%m-%d %H:%M:%S") << " "
                          << inet_ntoa(client_addr.sin_addr) << " "
                          << req.method << " " << req.path << " -> "
                          << selected.host << ":" << selected.port << std::endl;

                backend_fd = socket(AF_INET, SOCK_STREAM, 0);
                if (backend_fd < 0) {
                    std::cerr << "Failed to create backend socket for " << selected.host << ":" << selected.port << std::endl;
                    continue;
                }

                struct hostent *server = gethostbyname(selected.host.c_str());
                if (server == nullptr) {
                    std::cerr << "Failed to resolve backend host " << selected.host << std::endl;
                    close(backend_fd);
                    continue;
                }

                struct sockaddr_in backend_addr;
                backend_addr.sin_family = AF_INET;
                backend_addr.sin_port = htons(selected.port);
                memcpy(&backend_addr.sin_addr.s_addr, server->h_addr, server->h_length);

                if (connect(backend_fd, (struct sockaddr *)&backend_addr, sizeof(backend_addr)) < 0) {
                    std::cerr << "Failed to connect to backend " << selected.host << ":" << selected.port << std::endl;
                    close(backend_fd);
                    continue;
                }

                connected = true;
                backends[idx].active_connections++;
                break;
            }

            if (!connected) {
                std::cerr << "All backends failed, closing client connection" << std::endl;
                close(client_fd);
                continue;
            }

            send(backend_fd, buffer, bytes_read, 0);

            char response_buffer[4096] = {0};
            int response_bytes = read(backend_fd, response_buffer, 4096);
            while (response_bytes > 0) {
                send(client_fd, response_buffer, response_bytes, 0);
                response_bytes = read(backend_fd, response_buffer, 4096);
            }

            close(backend_fd);
            for (auto& b : backends) {
                if (b.host == selected.host && b.port == selected.port) {
                    b.active_connections--;
                    break;
                }
            }
        }
        
        close(client_fd);
    }

    close(server_fd);
    return 0;
}

