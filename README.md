# Mini Exchange Simulator

A simplified electronic exchange built with C++20.

### Stack

* C++20
* Boost.Asio
* ZeroMQ
* PostgreSQL
* Docker Compose
* GoogleTest

### Services

* Gateway
* Risk
* Matching Engine
* Market Data
* Persistence
* Client

The Matching Engine implements price-time priority with limit orders, partial fills, cancel, and modify.

### Run

```bash
docker compose up --build -d
docker compose run --rm client
```

### Build & Test

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

See [`exchange/docs/architecture.md`] for detailed architecture and design decisions.
