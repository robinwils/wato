# Multi-stage build for watod (dedicated game server)

# --- Stage 1: install vcpkg dependencies (cached until manifest changes) ---
FROM ubuntu:24.04 AS deps

RUN apt-get update && apt-get install -y \
        zip unzip git curl \
        build-essential ninja-build \
        autoconf libtool pkg-config \
        libgl1-mesa-dev ca-certificates \
    && rm -rf /var/lib/apt/lists/*

ARG CMAKE_VERSION=3.31.6
RUN curl -fsSL "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz" \
    | tar xz -C /usr/local --strip-components=1

WORKDIR /src

COPY vcpkg/ vcpkg/
COPY vcpkg/ vcpkg_overlay/
RUN vcpkg/bootstrap-vcpkg.sh -disableMetrics

COPY vcpkg.json vcpkg-configuration.json ./
ARG VCPKG_TRIPLET=x64-linux-release
RUN vcpkg/vcpkg install --triplet ${VCPKG_TRIPLET} --x-no-default-features

# --- Stage 2: build watod ---
FROM deps AS builder

COPY CMakeLists.txt CMakePresets.json ./
COPY cmake/ cmake/
COPY src/ src/

ARG VCPKG_TRIPLET=x64-linux-release
RUN cmake --preset linux-gcc-release \
        -DENABLE_CLIENT=OFF \
        -DENABLE_TESTS=OFF \
        -DVCPKG_MANIFEST_FEATURES="" \
        -DBUILD_SHARED_LIBS=OFF \
        -DVCPKG_INSTALLED_DIR=/src/vcpkg_installed \
        -DVCPKG_MANIFEST_INSTALL=OFF \
        -DVCPKG_TARGET_TRIPLET=${VCPKG_TRIPLET}

RUN cmake --build --preset linux-gcc-release --target watod --parallel

# --- Runtime stage ---
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /src/watod .
COPY src/watod/entrypoint.sh .
RUN chmod +x entrypoint.sh

EXPOSE 7777/udp

ENTRYPOINT ["./entrypoint.sh"]
