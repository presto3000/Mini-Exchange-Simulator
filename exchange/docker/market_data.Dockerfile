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
    && cmake --build build --target market_data_service


FROM ubuntu:24.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    libzmq5 ca-certificates \
    librdkafka-dev \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /src/build/bin/market_data_service \
    /usr/local/bin/market_data_service

EXPOSE 9600

ENTRYPOINT ["/usr/local/bin/market_data_service"]