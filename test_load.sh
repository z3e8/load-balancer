#!/bin/bash

for i in {1..50}; do
    response=$(curl -s http://localhost:8080/)
    echo "Request $i: $response"
done

