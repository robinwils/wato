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
FetchContent_Declare(
	entt
	GIT_REPOSITORY https://github.com/skypjack/entt
	GIT_TAG v3.14.0
	FIND_PACKAGE_ARGS NAMES entt
)

FetchContent_MakeAvailable(assimp)
FetchContent_MakeAvailable(glfw3)
FetchContent_MakeAvailable(entt)

message(STATUS "assimp directories '${assimp_INCLUDE_DIR}' '${assimp_BINARY_DIR}', populated: ${assimp_POPULATED}")
message(STATUS "GLFW directories '${glfw3_INCLUDE_DIR}' '${glfw3_BINARY_DIR}', populated: ${glfw3_POPULATED}")
message(STATUS "entt directories '${entt_SOURCE_DIR}' '${entt_BINARY_DIR}', populated: ${entt_POPULATED}")

set(CMAKE_CXX_FLAGS_DEBUG "-DBX_CONFIG_DEBUG")

include_directories(
	deps/bimg/include
	src
	deps/bx/include
	deps/bgfx/include
	deps/imgui
	src/imgui
	deps/iconfontcppheaders
	deps/glm
	${entt_SOURCE_DIR}/src
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
	src/core/ray.cpp
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

target_sources(wato
PUBLIC FILE_SET HEADERS
    BASE_DIRS src
    FILES
	src/components/color.hpp
	src/components/direction.hpp
	src/components/position.hpp
	src/components/rotation.hpp
	src/components/scale.hpp
	src/components/scene_object.hpp
	src/components/transform3d.hpp
	src/components/placement_mode.hpp
	src/components/tile.hpp
	src/components/imgui.hpp
	src/components/camera.hpp
	src/components/health.hpp
	src/core/model_loader.hpp
	src/core/tile.hpp
	src/core/cache.hpp
	src/core/sys.hpp
	src/core/ray.hpp
	src/core/action.hpp
	src/core/registry.hpp
	src/input/input.hpp
	src/renderer/bgfx_utils.hpp
	src/renderer/material.hpp
	src/renderer/mesh_primitive.hpp
	src/renderer/primitive.hpp
	src/renderer/plane_primitive.hpp
	src/systems/systems.hpp
)

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

add_custom_target(format-sources
	COMMAND clang-format -style=file -i $<TARGET_PROPERTY:wato,SOURCES>
		$<TARGET_PROPERTY:wato,HEADER_SET>
	COMMAND_EXPAND_LISTS
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
