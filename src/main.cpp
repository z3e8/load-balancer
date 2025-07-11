#include <iostream>
#include "../include/LoadBalancer.h"

int main(int argc, char* argv[]) {
    std::string config_file = "config.json";
    if (argc > 1) {
        config_file = argv[1];
    }

    LoadBalancer lb(8080);
    lb.load_config(config_file);
    lb.run();

    return 0;
}
