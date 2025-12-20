FROM gcr.io/oss-fuzz-base/base-builder:v1
RUN apt-get update && apt-get install -y \
        unzip \
        git \
        curl \
        build-essential \
        ninja-build \
        autoconf \
        libtool \
        pkg-config \
        libgl1-mesa-dev \
        libglu1-mesa-dev \
        libvulkan-dev \
        libxinerama-dev \
        libxcursor-dev \
        xorg-dev \
        libx11-dev \
        libwayland-dev \
        libxkbcommon-dev \
        wayland-protocols \
        ca-certificates \
        && rm -rf /var/lib/apt/lists/*

COPY . $SRC/wato
WORKDIR wato
COPY .clusterfuzzlite/build.sh $SRC/
