# Load Balancer

A lightweight C++ load balancer and reverse proxy that distributes HTTP requests across multiple backend servers.

## Build

```bash
mkdir build
cd build
cmake ..
make
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
