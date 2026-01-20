FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    gdb \
    lldb \
    valgrind \
    git \
    libspdlog-dev \
    libicu-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
