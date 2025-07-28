#!/bin/bash

if ! command -v wrk &> /dev/null; then
    echo "wrk not found. Install with: brew install wrk (macOS) or apt-get install wrk (Linux)"
    exit 1
fi

echo "Running load test with wrk..."
echo "1000 requests, 10 concurrent connections"
echo ""

wrk -t4 -c10 -d10s --latency http://localhost:8080/ > bench_results.txt 2>&1

echo "Results saved to bench_results.txt"
echo ""
cat bench_results.txt

