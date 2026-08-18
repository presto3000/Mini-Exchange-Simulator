# Mini Exchange Simulator

A C++20 low-latency architecture demo of a simplified electronic exchange.

The project implements a **price-time priority matching engine**, **pre-trade risk service**, and **client gateway**, connected through a small binary-framed TCP protocol.

The design prioritizes:

* Correct matching semantics
* Clear service boundaries
* Predictable single-threaded execution in the matching hot path
* Strongly typed domain values
* A realistic distributed architecture without unnecessary complexity

> **Note:** This is an educational architecture demonstration, not a production trading system.

## Architecture

```text
                     TCP :9000
Client ─────────────────────────────► Gateway
                                      │
                                      │ TCP :9200
                                      ▼
                                    Risk
                                      │
                                      │ TCP :9100
                                      ▼
                              Matching Engine
```

The gateway can also run the order-processing path in-process for development and testing.

```text
Client
  │
  │ TCP :9000
  ▼
Gateway
  │
  ├── in-process ───────────────► Risk
  │
  └── TCP :9200 ────────────────► Risk
                                   │
                                   │ TCP :9100
                                   ▼
                            Matching Engine
```

### Components

| Component           |   Port | Responsibility                                                               |
| ------------------- | -----: | ---------------------------------------------------------------------------- |
| **Gateway**         | `9000` | Accepts client connections and dispatches orders                             |
| **Risk**            | `9200` | Performs pre-trade risk checks and forwards approved orders                  |
| **Matching Engine** | `9100` | Maintains the order book, performs price-time matching, and generates trades |
| **Common**          |      — | Shared domain types, matching logic, protocol, networking, and logging       |

## Core Design

### One engine, one order book

Each `MatchingEngine` owns exactly one `OrderBook` for one symbol.

The matching engine is intentionally **single-threaded**. There are no locks in the matching hot path.

Scaling is achieved by running multiple matching engines, typically one per symbol or symbol partition, rather than introducing synchronization into the core matching logic.

```text
Symbol A ──► Matching Engine A ──► OrderBook A
Symbol B ──► Matching Engine B ──► OrderBook B
Symbol C ──► Matching Engine C ──► OrderBook C
```

This keeps the matching algorithm deterministic and makes its concurrency model explicit.

### Strong domain types

The common library uses dedicated value types for important domain concepts:

* `Price`
* `Quantity`
* `OrderId`
* `BuyingPower`

Prices are represented as **integer ticks** rather than floating-point values. This avoids floating-point rounding issues and makes price comparisons deterministic.

### Value-oriented hot path

Orders and related domain objects are plain value types rather than polymorphic class hierarchies.

The goal is to keep the matching path simple, predictable, and easy to reason about.

## Matching Rules

The matching engine implements strict **price-time priority**.

### New orders

* Aggressive orders match immediately against the opposite side of the book.
* Matches are executed according to price priority first, then FIFO time priority.
* Any remaining quantity rests on the book.
* A single incoming order may match against multiple price levels.

### Cancels

Cancellation is **idempotent**.

Attempting to cancel an order that has already been filled or cancelled is treated as a normal case rather than an error.

### Modify

Modification follows these rules:

| Modification            | Time Priority |
| ----------------------- | ------------- |
| Price changes           | Lost          |
| Quantity increases      | Lost          |
| Quantity decreases only | Preserved     |

A modification that loses priority is handled as a remove/reinsert operation and is subsequently eligible for matching again.

A quantity reduction that does not change the price preserves the order's existing queue position.

## Common Library

The `common` library contains the core exchange building blocks:

```text
common/
├── Strong domain types
├── Order
├── PriceLevel
├── OrderBook
├── MatchingEngine
├── Trade
├── Binary protocol
├── Boost.Asio networking
└── Structured logging
```

### Binary Protocol

Communication between services uses a **length-prefixed binary protocol** implemented on top of TCP.

Boost.Asio provides the networking layer and reusable frame server/client components.

The protocol is intentionally small and explicit so that service boundaries remain easy to inspect and test.

## Project Layout

```text
exchange/
├── common/               # Shared types, order book, matching engine, protocol, networking
├── gateway/              # Client-facing gateway
├── risk/                 # Pre-trade risk service
├── matching_engine/      # Matching engine service
├── tests/                # Unit and integration tests
├── docker/               # Dockerfiles
├── cmake/                # CMake helpers and compiler warnings
└── CMakeLists.txt
```

## Building

### Prerequisites

* CMake `>= 3.25`
* C++20-compatible compiler
* Boost `>= 1.83`
* GoogleTest for the test suite

Boost is expected to be available as a system dependency for the native build.

### Native Build

```bash
cd exchange

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

Run the tests:

```bash
ctest --test-dir build --output-on-failure
```

Individual test executables can also be run directly from the build directory.

### Useful Targets

```text
common
gateway
risk
matching_engine
*_tests
```

## Docker

Build the base image:

```bash
docker build \
  -f docker/base.Dockerfile \
  -t mini-exchange-base .
```

Start the full stack:

```bash
docker compose up --build
```

The services are exposed on:

```text
Gateway          localhost:9000
Risk             localhost:9200
Matching Engine  localhost:9100
```

Startup races between containers are handled with simple retry and backoff logic.

For a production deployment, this would typically be replaced or supplemented by proper service health checks and orchestration.

## Configuration

Service connectivity can be configured through environment variables.

Examples include:

```text
GATEWAY_ORDER_PROCESSOR=remote|inprocess
RISK_HOST=<host>
MATCHING_ENGINE_HOST=<host>
```

The `inprocess` gateway mode is useful for local development and tests, while the `remote` mode exercises the service-to-service TCP architecture.

## Testing

The project includes unit and integration tests covering the core exchange behavior.

Important matching scenarios include:

* Price priority
* FIFO time priority
* Aggressive order matching
* Partial fills
* Orders resting on the book
* Multi-level matching
* Idempotent cancellation
* Quantity-reduction modifications
* Priority-resetting modifications
* Risk rejection
* Binary protocol framing
* Service-to-service communication

Run the complete test suite with:

```bash
ctest --test-dir build --output-on-failure
```

## Static Analysis and Compiler Warnings

Compiler warnings are configured through:

```text
cmake/CompilerWarnings.cmake
```

To enable `clang-tidy`:

```bash
cmake -S . -B build \
  -DEXCHANGE_ENABLE_CLANG_TIDY=ON
```

Then build normally:

```bash
cmake --build build -j
```

## Design Principles

The project intentionally favors a few principles over feature completeness:

### No floating-point prices

All prices are represented using integer ticks.

This makes equality and ordering deterministic and avoids the ambiguity introduced by floating-point arithmetic.

### No locks in the matching engine

The matching engine is single-threaded by design.

Concurrency is handled at the architecture level by partitioning work across multiple engines rather than protecting the order book with mutexes.

### Explicit ownership

A matching engine owns exactly one order book.

This makes the state that must remain consistent during matching explicit and local.

### Simple value types

The hot path favors structs and value types over inheritance and polymorphism.

This keeps the core model compact and makes the matching algorithm easier to test.

### Realistic service boundaries

Gateway, risk, and matching are separate services in the remote configuration.

This provides a realistic example of how exchange components can be separated without attempting to reproduce the complexity of a production exchange.

## Future Extensions

The CMake structure leaves room for additional modules, including:

* Market data dissemination
* Persistence / journaling
* Multi-symbol orchestration
* Additional order types
* Replay tooling
* Metrics and observability
* More advanced risk controls

These are intentionally outside the scope of the current implementation.

## Limitations

This project is **not intended to be a production-ready exchange**.

It intentionally omits or simplifies areas such as:

* Durable event persistence
* Crash recovery
* Distributed consensus
* High-availability failover
* Authentication and authorization
* TLS
* Production-grade service discovery
* Advanced market-data distribution
* Comprehensive exchange order types
* Production-grade risk management
* Kernel-bypass or specialized network I/O

The goal is to demonstrate the architecture and correctness of the core exchange concepts in a compact C++20 codebase.

## License

MIT License. See [`LICENSE`](LICENSE) for details.

---

**Mini Exchange Simulator** is an educational project focused on demonstrating a correct matching engine, strongly typed exchange domain model, pre-trade risk checks, and lightweight service-oriented architecture in modern C++20.
