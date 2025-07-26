#ifndef CONNECTION_POOL_H
#define CONNECTION_POOL_H

#include <vector>
#include <string>
#include <map>
#include <mutex>

// Reuses backend connections to avoid overhead of creating new sockets
// Maintains a pool of open connections per backend
class ConnectionPool {
public:
    int get_connection(const std::string& host, int port);
    void return_connection(const std::string& host, int port, int fd);
    void close_all();
    
private:
    std::map<std::string, std::vector<int>> pool;
    std::mutex mutex;
    std::string make_key(const std::string& host, int port);
};

#endif

