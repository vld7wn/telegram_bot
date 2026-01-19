FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libssl-dev \
    libcurl4-openssl-dev \
    libsqlite3-dev \
    libboost-system-dev \
    git \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    ninja-build \
    && rm -rf /var/lib/apt/lists/*

# Install vcpkg and dependencies (without xlnt - using CSV fallback)
WORKDIR /opt
RUN git clone https://github.com/Microsoft/vcpkg.git && \
    ./vcpkg/bootstrap-vcpkg.sh && \
    ./vcpkg/vcpkg install tgbot-cpp nlohmann-json cpp-httplib

# Copy source code
WORKDIR /app
COPY . .

# Build
RUN mkdir build && cd build && \
    cmake .. -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake && \
    cmake --build . --config Release

# Create directories
RUN mkdir -p db logs

# Copy config files
RUN cp -r json-cfg build/json-cfg

WORKDIR /app/build

# Expose HTTP API port
EXPOSE 8080

# Run bot
CMD ["./my_telegram_bot"]
