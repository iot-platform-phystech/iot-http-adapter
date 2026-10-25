
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential cmake git python3-pip \
    libjsoncpp-dev uuid-dev zlib1g-dev \
    libssl-dev libbrotli-dev libpq-dev \
    && rm -rf /var/lib/apt/lists/*

RUN python3 -m pip install --no-cache-dir cmake==3.31.6

WORKDIR /opt
RUN git clone https://github.com/drogonframework/drogon.git && \
    cd drogon && git checkout v1.9.3 && \
    git submodule update --init && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_POSTGRESQL=ON && \
    make -j2 && make install && ldconfig

WORKDIR /app
COPY . .
RUN rm -rf build && \
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=/usr/local && \
    cmake --build build -j2

FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libjsoncpp25 uuid-runtime zlib1g \
    libssl3 libbrotli1 libpq5 ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/local/lib/ /usr/local/lib/
RUN ldconfig

WORKDIR /app
COPY --from=builder /app/build/iot-http-adapter ./iot-http-adapter

ENV IOT_CORE_URL=http://core:8000
ENV IOT_ADAPTER_HOST=0.0.0.0

EXPOSE 8080

CMD ["./iot-http-adapter"]
