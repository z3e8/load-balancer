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

struct HttpRequest {
    std::string method;
    std::string path;
    std::unordered_map<std::string, std::string> headers;
};

struct Backend {
    std::string host;
    int port;
};

int main() {
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
    backends.push_back({"localhost", 8001});
    backends.push_back({"localhost", 8002});
    backends.push_back({"localhost", 8003});

    std::cout << "Server listening on port 8080" << std::endl;
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

            Backend selected = backends[round_robin_counter % backends.size()];
            round_robin_counter++;

            std::time_t now = std::time(nullptr);
            std::tm* timeinfo = std::localtime(&now);
            std::cout << std::put_time(timeinfo, "%Y-%m-%d %H:%M:%S") << " "
                      << inet_ntoa(client_addr.sin_addr) << " "
                      << req.method << " " << req.path << " -> "
                      << selected.host << ":" << selected.port << std::endl;

            int backend_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (backend_fd < 0) {
                std::cerr << "Failed to create backend socket" << std::endl;
                close(client_fd);
                continue;
            }

            struct hostent *server = gethostbyname(selected.host.c_str());
            if (server == nullptr) {
                std::cerr << "Failed to resolve backend host" << std::endl;
                close(backend_fd);
                close(client_fd);
                continue;
            }

            struct sockaddr_in backend_addr;
            backend_addr.sin_family = AF_INET;
            backend_addr.sin_port = htons(selected.port);
            memcpy(&backend_addr.sin_addr.s_addr, server->h_addr, server->h_length);

            if (connect(backend_fd, (struct sockaddr *)&backend_addr, sizeof(backend_addr)) < 0) {
                std::cerr << "Failed to connect to backend" << std::endl;
                close(backend_fd);
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
        }
        
        close(client_fd);
    }

    close(server_fd);
    return 0;
}

