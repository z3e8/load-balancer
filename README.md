# Load Balancer

A lightweight C++ load balancer and reverse proxy that distributes HTTP requests across multiple backend servers.

## Features

- Round-robin and least-connections load balancing algorithms
- Health checking with automatic backend removal
- Connection pooling for improved performance
- Configurable via JSON config file
- Thread-safe implementation
- Graceful shutdown on SIGINT/SIGTERM

## Build

```bash
mkdir build
cd build
cmake ..
make
```

## Docker

Build and run with Docker:

```bash
docker build -t load-balancer .
docker run -p 8080:8080 -v $(pwd)/config.json:/app/config.json load-balancer
```

## Run Demo

1. Start backend servers:
```bash
./start_backends.sh
```

2. Run load balancer:
```bash
./build/load_balancer
```

3. Test with load script:
```bash
./test_load.sh
```

4. Test health checks:
```bash
./test_health.sh
```

## Configuration

Edit `config.json` to configure backends and load balancing strategy:

```json
{
  "backends": [
    {"host": "localhost", "port": 8001},
    {"host": "localhost", "port": 8002},
    {"host": "localhost", "port": 8003}
  ],
  "strategy": "round_robin",
  "health_check_interval": 10
}
```

Supported strategies: `round_robin`, `least_connections`
