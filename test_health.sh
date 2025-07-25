#!/bin/bash

echo "Starting backends..."
./start_backends.sh
sleep 2

echo "Testing load balancer with all backends up..."
for i in {1..10}; do
    curl -s http://localhost:8080/ > /dev/null
done

echo "Killing backend on port 8002..."
pkill -f "backend_server.py 8002"
sleep 12

echo "Testing load balancer after killing backend..."
for i in {1..10}; do
    response=$(curl -s http://localhost:8080/)
    echo "Request $i: $response"
done

echo "Test complete"

