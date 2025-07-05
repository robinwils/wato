# Dependencies

Depending on the OS you will need different tools for compiling, see individual sections below.

But common dependencies are:
- CMake
- python
- git

# Build

first clone repo and submodules:
```bash
git clone --recurse-submodules -j8 https://github.com/robinwils/wato.git
# or ssh
git clone --recurse-submodules -j8 git@github.com:robinwils/wato.git
```

Then fetch assets with Git LFS (install git lfs first):
```bash
git lfs pull
```
## Icon fonts

Install PyYAML, requests and generate fonts
```
pip install PyYAML requests
# execute script in subdir
cd deps/iconfontcppheaders
python GenerateIconFontCppHeaders.py --ttf2headerC
```

## Windows

### Dependencies

Most dependencies are handled through vcpkg, but you still need:
- MSVC compiler (Visual Studio Build Tools)
- Ninja

### VS options

1. Add `/Zc:__cplusplus` in *Configuration Properties > C/C++ > Command Line*
2. Use the same config for Runtime Library in *Configuration Properties > C/C++ > Code Generation > Runtime Library*
3. Link libraries:
```
glfw3.lib or glfw3_mt.lib
bgfxDebug.lib
bxDebug.lib
bimgDebug.lib
```

## Linux

### Dependencies

- GLFW
- lib X11
- lib vulkan
- lib sodium

#### Debian/Ubuntu

Filter unwanted (already installed or installed through another method) from:
```bash
apt-get install -y \
    zip \
    unzip \
    curl \
    ca-certificates \
    build-essential \
    ninja-build \
    pkg-config \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libxinerama-dev \
    libxcursor-dev \
    xorg-dev \
    libx11-dev \
    libwayland-dev \
    libxkbcommon-dev \
    wayland-protocols \
    libsodium-dev
```
### Build

Build is done using cmake with presets:

```bash
cmake --preset unixlike-clang-debug-sccache
cmake --build out/build/unixlike-clang-debug-sccache -j $(nproc)
```

## Shaders

Shaders are compiled using a makefile, they are kept in the same place as bgfx shader files

You need to compile shaderc binary from the bgfx solution (tools > shaderc > shaderc) in release mode

```
$ make -C shaders linux-shaders DEBUG=1
$ make -C shaders windows-shaders
$ make -C shaders osx-shaders
```

## Vagrant

Careful with firewalls, disable before `vagrant up`

```
apt install nfs-kernel-server rpcbind
vagrant plugin install vagrant-libvirt
vagrant plugin install winrm
vagrant plugin install winrm-fs
vagrant plugin install winrm-elevated (this additional error showed after the first two were installed)
```
