cmake_minimum_required(VERSION 3.31)

project(WaTo)

set(CMAKE_CXX_STANDARD 20)

find_package(ZLIB REQUIRED)

include(FetchContent)
FetchContent_Declare(
	assimp
	GIT_REPOSITORY https://github.com/assimp/assimp.git
	GIT_TAG v5.4.3
	FIND_PACKAGE_ARGS NAMES assimp
)
FetchContent_Declare(
	glfw3
	GIT_REPOSITORY https://github.com/glfw/glfw
	GIT_TAG 3.4
	FIND_PACKAGE_ARGS NAMES glfw3
)

FetchContent_MakeAvailable(assimp)
FetchContent_MakeAvailable(glfw3)

message(STATUS "assimp directories '${assimp_INCLUDE_DIR}' '${assimp_BINARY_DIR}', populated: ${assimp_POPULATED}")
message(STATUS "GLFW directories '${glfw3_INCLUDE_DIR}' '${glfw3_BINARY_DIR}', populated: ${glfw3_POPULATED}")


set(CMAKE_CXX_FLAGS_DEBUG "-DBX_CONFIG_DEBUG")

include_directories(
	deps/bimg/include
	src
	deps/bx/include
	deps/bgfx/include
	deps/imgui
	src/imgui
	deps/iconfontcppheaders
	deps/entt/src
	deps/glm
	${assimp_INCLUDE_DIR}/include
	${assimp_SOURCE_DIR}/include
)

if(WIN32)
	include_directories(
		deps/bx/include/compat/msvc
	)
	link_directories(
		deps/bgfx/.build/win64_vs2022/bin
		deps/glfw/lib-vc2022
	)
elseif(UNIX)
	include_directories(
		deps/bx/include/compat/linux
	)
	link_directories(
		deps/bgfx/.build/linux64_gcc/bin
	)
endif()

# Source files
set(SOURCES
	deps/imgui/imgui.cpp
	deps/imgui/imgui_draw.cpp
	deps/imgui/imgui_tables.cpp
	deps/imgui/imgui_widgets.cpp
	src/core/sys.cpp
	src/renderer/material.cpp
	src/systems/render.cpp
	src/systems/imgui.cpp
	src/systems/input.cpp
	src/systems/action.cpp
	src/systems/camera.cpp
	src/renderer/bgfx_utils.cpp
	src/core/model_loader.cpp
	src/core/registry.cpp
	src/imgui/imgui_helper.cpp
	src/input/input.cpp
	src/main.cpp
)

# Add executable
add_executable(wato ${SOURCES})

# Link libraries
target_link_libraries(wato
	bgfxDebug
	bxDebug
	bimg_decodeDebug
	bimg_encodeDebug
	bimgDebug
)

if(WIN32)
	target_link_libraries(wato
		glfw3_mt
	)
elseif(UNIX)
	target_link_libraries(wato
		glfw
		assimp
		X11
		vulkan
		GL
		GLU
		ZLIB::ZLIB
	)
endif()
