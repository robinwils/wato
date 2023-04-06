# Build

first clone repo and submodules:
```
git clone git@github.com:robinwils/wato.git
cd wato
git submodule update --init --recursive
```

## Windows
### GLFW
Build VS solution using cmake and then build GLFW inside visual studio

### BGFX

Build VS solution:
```
cd .\deps\bgfx
..\bx\tools\bin\windows\genie --with-examples --with-tools vs2022
```

Open solution in visual studio and build bgfx, bx and bimg

### GLFW

glfw has precompiled binaries, download them from the official website and copy the folder to deps/glfw

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

## Shaders

Shaders are compiled using a makefile, they are kept in the same place as bgfx shader files

```
$ cd shaders/
$ make
```