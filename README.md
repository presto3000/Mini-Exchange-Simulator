# Mini-Exchange-Simulator

A C++20 low-latency architecture demo of a simplified exchange.

It implements a correct **price-time priority matching engine**, a **risk service**, and a **client gateway**, wired together with a binary framed TCP protocol. The design prioritises clarity, correct matching semantics, and realistic service boundaries over raw feature completeness.

> One MatchingEngine owns exactly one OrderBook (one symbol) and is single-threaded by design. Scaling is done by running multiple engines, never by adding locks inside the hot path.

## Architecture

```
Client ──TCP:9000──► Gateway ──TCP:9200──► Risk ──TCP:9100──► Matching Engine
                         │                   │
                    (in-process mode)   (pre-trade checks)
```

|Component|Port|Responsibility|
|-|-|-|
|**Gateway**|9000|Accepts client connections, dispatches orders|
|**Risk**|9200|Max size / buying-power checks, forwards to ME|
|**Matching Engine**|9100|Price-time priority matching, generates trades|

### Core library (`common`)

* Strong types (`Price`, `Quantity`, `OrderId`, `BuyingPower`) – integer ticks only
* `Order`, `PriceLevel`, `OrderBook`, `MatchingEngine`, `Trade`
* Length-prefixed binary protocol + Boost.Asio frame server/client
* Structured logging

### Matching rules

* Strict price-time (FIFO) priority
* Aggressive orders match immediately; remainder rests
* Cancel is idempotent (already-filled/cancelled is a normal case)
* Modify:

  * Price change **or** quantity increase → loses time priority (re-insert + re-match)
  * Quantity decrease only → keeps queue position

## Quick start

### Native build

```bash
# Prerequisites: CMake ≥ 3.25, C++20 compiler, Boost ≥ 1.83 (system)
cd exchange
cmake -S . -B build -DCMAKE\\\_BUILD\\\_TYPE=Debug
cmake --build build -j
ctest --test-dir build          # or run the individual test binaries
```

Useful targets: `common`, `gateway`, `risk`, `matching\\\_engine`, and the `\\\*\\\_tests` executables.

### Docker

```bash
# Build base image (compiles the common library)
docker build -f docker/base.Dockerfile -t mini-exchange-base .

# Full stack
docker compose up --build
```

Services become available on:

* Gateway -> `localhost:9000`
* Risk -> `localhost:9200`
* Matching Engine -> `localhost:9100`

Environment variables control connectivity (`GATEWAY\\\_ORDER\\\_PROCESSOR=remote|inprocess`, `RISK\\\_HOST`, `MATCHING\\\_ENGINE\\\_HOST`, etc.).

## Project layout

```
exchange/
├── common/               # Shared types, order book, matching engine, protocol, net
├── gateway/              # Client-facing service
├── risk/                 # Pre-trade risk checks
├── matching\\\_engine/      # Matching service
├── tests/                # Unit + integration tests (GoogleTest)
├── docker/               # Dockerfiles
├── cmake/                # Compiler warnings, etc.
└── CMakeLists.txt
```

## Design notes

* **No floating-point prices** – everything is integer ticks.
* **No locks inside the matching engine** – single-threaded per symbol is intentional.
* **Value types preferred** on the hot path (orders are plain structs, not polymorphic).
* **Startup races** in Docker are handled with simple retry + backoff (production systems would use health-checks).
* Future modules (market data, persistence, multi-symbol orchestration, richer order types) are stubbed in the CMake structure.

## Building with clang-tidy / warnings

```bash
cmake -S . -B build -DEXCHANGE\\\_ENABLE\\\_CLANG\\\_TIDY=ON
```

Compiler warnings are enabled via `cmake/CompilerWarnings.cmake`.

## License

MIT License – see [LICENSE](LICENSE).

\---

*This is an educational / architecture-demonstration project, not a production exchange.*

```




