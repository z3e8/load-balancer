#!/bin/bash

echo "Starting backends..."
./start_backends.sh
sleep 2

echo "Testing load balancer with all backends up..."
for i in {1..10}; do
    curl -s http://localhost:8080/ > /dev/null
done

echo "Killing backend on port 8002..."
kill_time=$(date +%s)
pkill -f "backend_server.py 8002"

echo "Testing load balancer after killing backend..."
recovery_time=0
found_8002=false
for i in {1..20}; do
    response=$(curl -s http://localhost:8080/)
    echo "Request $i: $response"
    
    # check if we still get 8002 responses
    if echo "$response" | grep -q "8002"; then
        found_8002=true
    else
        if [ "$found_8002" = true ]; then
            current_time=$(date +%s)
            recovery_time=$((current_time - kill_time))
            echo "Backend 8002 stopped responding after $recovery_time seconds"
            found_8002=false
        fi
    fi
    
    sleep 1
done

if [ "$recovery_time" -eq 0 ]; then
    current_time=$(date +%s)
    recovery_time=$((current_time - kill_time))
    echo "Recovery time: $recovery_time seconds (may still be routing to 8002)"
else
    echo "Recovery time: $recovery_time seconds"
fi

echo "Test complete"

