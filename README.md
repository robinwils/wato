# WaTo

3D PvP Tower Defense game built with C++20. Features client-server architecture with deterministic lockstep networking, custom bit-level serialization, and an ECS using EnTT.

## Getting Started

```bash
git clone --recurse-submodules -j8 git@github.com:robinwils/wato.git
cd wato
git lfs pull
```

## Prerequisites

- CMake 3.29+
- Ninja
- C++20 compiler (GCC 12+, Clang 16+, or MSVC 17+)
- Git LFS
- vcpkg (included as submodule)

### Linux (Debian/Ubuntu)

```bash
sudo apt-get install -y \
    zip unzip curl ca-certificates git git-lfs \
    build-essential ninja-build cmake pkg-config \
    autoconf automake libtool \
    libgl1-mesa-dev libglu1-mesa-dev libvulkan-dev \
    libxinerama-dev libxcursor-dev xorg-dev libx11-dev \
    libwayland-dev libxkbcommon-dev wayland-protocols
```

`autoconf`, `automake`, `libtool` are needed by vcpkg to build libsodium from source.
The `libvulkan-dev`, X11 and Wayland packages are only needed when building the client (`ENABLE_CLIENT=ON`).

### macOS

Xcode Command Line Tools provide clang and system headers. Install additional tools via Homebrew:

```bash
xcode-select --install
brew install cmake ninja autoconf automake libtool
```

### Windows

1. Install [Visual Studio 2022](https://visualstudio.microsoft.com/) (Community or Build Tools) with the **"Desktop development with C++"** workload
2. Install [CMake 3.29+](https://cmake.org/download/) and [Ninja](https://ninja-build.org/) (or via `choco install cmake ninja`)
3. Install the [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)

Run builds from a **Developer Command Prompt** (or use `vcvarsall.bat`) so MSVC is on PATH.

## Build

CMake presets handle all configuration:

```bash
# Configure
cmake --preset unixlike-gcc-debug        # Linux GCC
cmake --preset unixlike-clang-debug      # Linux/macOS Clang
cmake --preset windows-msvc-debug        # Windows MSVC

# Build
cmake --build --preset <preset-name>
```

Append `-sccache` to any preset name to use sccache (e.g. `unixlike-clang-debug-sccache`).

Replace `debug` with `release` for optimized builds.

### Build Targets

| Target | Description | Option |
|--------|-------------|--------|
| `wato` | Game client | `ENABLE_CLIENT=ON` |
| `watod` | Dedicated server | `ENABLE_SERVER=ON` |
| `wato_tests` | Test suite | `ENABLE_TESTS=ON` |

### Tests

```bash
./out/build/<preset-name>/test/wato_tests
```

## Docker Compose

Three services run via `docker-compose.yml` at the project root:

| Service | Description | Port |
|---------|-------------|------|
| `watohttpd` | PocketBase backend (auth, matchmaking) | internal :8090 |
| `caddy` | Reverse proxy with automatic TLS | :443 |
| `watod` | Dedicated game server | :7777/udp |

### Environment Variables

All variables come from the shell environment (use [direnv](https://direnv.net/) with `.envrc`):

| Variable | Description | Dev default |
|----------|-------------|-------------|
| `DOMAIN` | Domain for Caddy TLS | `localhost` |
| `CADDYFILE` | Path to Caddyfile | `./Caddyfile.dev` |
| `AUTOMIGRATE` | Auto-run PocketBase migrations | `1` |
| `ADMIN_EMAIL` | PocketBase admin email | `admin@wato.com` |
| `ADMIN_PASSWORD` | PocketBase admin password | `Wato1234!` |
| `PB_TOKEN` | PocketBase admin token for watod | (set in .envrc) |

### Quick Start

```bash
direnv allow
docker compose up --build

# Verify
docker compose ps                          # 3 services running
curl -k https://localhost/api/health       # PocketBase OK
docker compose logs watod                  # watod connected
```

### Trusting Caddy's CA on the Host

In development, Caddy generates a local CA. For the game client (running outside Docker) to connect via HTTPS:

```bash
docker compose cp caddy:/data/caddy/pki/authorities/local/root.crt /tmp/caddy-root.crt

# Linux
sudo cp /tmp/caddy-root.crt /usr/local/share/ca-certificates/caddy-local.crt
sudo update-ca-certificates

# macOS
sudo security add-trusted-cert -d -r trustRoot -k /Library/Keychains/System.keychain /tmp/caddy-root.crt
```

### Dev vs Prod

| | Dev | Prod |
|---|---|---|
| `DOMAIN` | `localhost` | `wato.example.com` |
| `CADDYFILE` | `./Caddyfile.dev` | `./Caddyfile.prod` |
| TLS | Internal (self-signed) | Let's Encrypt |
| `AUTOMIGRATE` | `1` | _(empty)_ |
