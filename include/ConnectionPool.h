#ifndef CONNECTION_POOL_H
#define CONNECTION_POOL_H

#include <vector>
#include <string>
#include <map>

class ConnectionPool {
public:
    int get_connection(const std::string& host, int port);
    void return_connection(const std::string& host, int port, int fd);
    void close_all();
    
private:
    std::map<std::string, std::vector<int>> pool;
    std::string make_key(const std::string& host, int port);
};

#endif

