FROM ubuntu:24.04 AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build git ca-certificates \
    libboost-all-dev \
    libzmq3-dev \
    libpqxx-dev \
    libpq-dev \
    librdkafka-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DEXCHANGE_ENABLE_TESTS=OFF \
    && cmake --build build --target persistence_service


FROM ubuntu:24.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    libzmq5 \
    librdkafka-dev \
    libpqxx-7.8t64 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /src/build/bin/persistence_service \
    /usr/local/bin/persistence_service

ENTRYPOINT ["/usr/local/bin/persistence_service"]