#!/bin/bash

python3 backend_server.py 8001 &
python3 backend_server.py 8002 &
python3 backend_server.py 8003 &

echo "Started 3 backend servers on ports 8001, 8002, 8003"
echo "PIDs:"
jobs -p

