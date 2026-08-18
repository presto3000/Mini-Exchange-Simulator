FROM ubuntu:24.04 AS build
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build git ca-certificates libboost-system-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DEXCHANGE_ENABLE_TESTS=OFF \
    && cmake --build build --target gateway_service

FROM ubuntu:24.04 AS runtime
RUN apt-get update && apt-get install -y --no-install-recommends \
    libboost-system1.83.0 ca-certificates \
    && rm -rf /var/lib/apt/lists/*
COPY --from=build /src/build/bin/gateway_service /usr/local/bin/gateway_service
EXPOSE 9000
ENTRYPOINT ["/usr/local/bin/gateway_service"]