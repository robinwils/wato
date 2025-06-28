FROM ubuntu:24.04

# Install system dependencies
RUN apt-get update && \
    apt-get install -y \
        zip \
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
        python3 \
        python3-pip \
        python3-venv \
    && rm -rf /var/lib/apt/lists/*
