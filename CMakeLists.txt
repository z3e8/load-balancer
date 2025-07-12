cmake_minimum_required(VERSION 3.10)
project(load_balancer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(load_balancer 
    src/main.cpp
    src/LoadBalancer.cpp
    src/BackendPool.cpp
    src/HttpParser.cpp
    src/Logger.cpp
    src/ConnectionPool.cpp
)

