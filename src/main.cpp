#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

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

    std::cout << "Server listening on port 8080" << std::endl;

    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        
        if (client_fd < 0) {
            continue;
        }

        std::cout << "Client connected from " << inet_ntoa(client_addr.sin_addr) << std::endl;
        
        char buffer[4096] = {0};
        int bytes_read = read(client_fd, buffer, 4096);
        if (bytes_read > 0) {
            std::cout << "HTTP Request:" << std::endl;
            std::cout << buffer << std::endl;
        }
        
        close(client_fd);
    }

    close(server_fd);
    return 0;
}

