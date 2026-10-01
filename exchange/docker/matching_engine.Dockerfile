FROM ubuntu:24.04 AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
    libzmq3-dev \
    libpqxx-dev \
    libpq-dev \
    librdkafka-dev \
    pkg-config \
    build-essential \
    cmake \
    ninja-build \
    git \
    ca-certificates \
    libboost-system-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

COPY . .

RUN cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DEXCHANGE_ENABLE_TESTS=OFF \
    && cmake --build build --target matching_engine_service


FROM ubuntu:24.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    libzmq5 \
    libpqxx-7.8 \
    libpq5 \
    librdkafka1 \
    librdkafka++1 \
    libboost-system1.83.0 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /src/build/bin/matching_engine_service \
    /usr/local/bin/matching_engine_service

EXPOSE 9100 9500

ENTRYPOINT ["/usr/local/bin/matching_engine_service"]
