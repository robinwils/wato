# Build

first clone repo and submodules:
```bash
git clone git@github.com:robinwils/wato.git
cd wato
git submodule update --init --recursive
```

Then fetch assets with Git LFS (install git lfs first):
```bash
git lfs pull
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

### Assimp
there is a WIP CMakeLists.txt that will generate assimp files

```bash
cmake -B 'Build' .
cmake --build .
```

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

on debian
```bash
sudo apt install libglfw3-dev libx11-dev libvulkan-dev
```

### Build

Build is done using cmake:

```bash
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1 .
```

`CMAKE_EXPORT_COMPILE_COMMANDS` is used for generating the compile json DB for LSP usage (clangd)
`CMAKE_CXX_COMPILER_LAUNCHER=ccache` can be used to improve build times.

## Icon fonts

`TODO: Automate this`

Go in the iconfontheaders, apply this patch:
```
diff --git a/GenerateIconFontCppHeaders.py b/GenerateIconFontCppHeaders.py
index c721a0d..6fef4f6 100644
--- a/GenerateIconFontCppHeaders.py
+++ b/GenerateIconFontCppHeaders.py
@@ -708,7 +708,7 @@ class LanguageGo( Language ):

 fonts = [ FontFA4, FontFA5, FontFA5Brands, FontFA5Pro, FontFA5ProBrands, FontFA6, FontFA6Brands, FontFK, FontMD, FontKI, FontFAD, FontCI ]
 languages = [ LanguageC, LanguageCSharp, LanguagePython, LanguageGo ]
-ttf2headerC = False # convert ttf files to C and C++ headers
+ttf2headerC = True # convert ttf files to C and C++ headers

 logging.basicConfig( format='%(levelname)s : %(message)s', level = logging.INFO )
```

Install PyYAML and generate fonts
```
$ pip install PyYAML
$ python .\GenerateIconFontCppHeaders.py
```

## Shaders

Shaders are compiled using a makefile, they are kept in the same place as bgfx shader files

You need to compile shaderc binary from the bgfx solution (tools > shaderc > shaderc) in release mode

```
$ make -C shaders linux-shaders
$ make -C shaders windows-shaders
$ make -C shaders osx-shaders
```

