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

### VS options

1. Add `/Zc:__cplusplus` in *Configuration Properties > C/C++ > Command Line*
2. Use `Multi-threaded Debug DLL (/MDd)` in *Configuration Properties > C/C++ > Code Generation > Runtime Library* (bgfx generates a different one than glfw)
3. Link libraries:
```
glfw3.lib
bgfxDebug.lib
bxDebug.lib
bimgDebug.lib
```
