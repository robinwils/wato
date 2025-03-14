cmake_minimum_required(VERSION 3.31)

project(WaTo)

set(CMAKE_CXX_STANDARD 20)

find_package(ZLIB REQUIRED)

option(DEBUG_ENABLE "Enable debug features" OFF)

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
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm
  GIT_TAG 0.9.9.8
  FIND_PACKAGE_ARGS NAMES glm
)
FetchContent_Declare(
  reactphysics3d
  GIT_REPOSITORY https://github.com/DanielChappuis/reactphysics3d
  GIT_TAG v0.10.2
  FIND_PACKAGE_ARGS NAMES reactphysics3d
)

FetchContent_MakeAvailable(assimp)
FetchContent_MakeAvailable(glfw3)
FetchContent_MakeAvailable(entt)
FetchContent_MakeAvailable(glm)
FetchContent_MakeAvailable(reactphysics3d)

set(CMAKE_CXX_FLAGS_DEBUG "-DBX_CONFIG_DEBUG")


if(DEBUG_ENABLE)
  set(WATO_DEBUG 1)
else()
  set(WATO_DEBUG 0)
endif()

# Configure the file using the provided (or default) value
configure_file(config.h.in config.h @ONLY)

add_executable(wato)

target_include_directories(wato
  PRIVATE
  deps/bimg/include
  src
  deps/bx/include
  deps/bgfx/include
  deps/imgui
  src/imgui
  deps/iconfontcppheaders
  ${glm_SOURCE_DIR}
  ${reactphysics3d_SOURCE_DIR}/include
)

if(DEFINED glfw3_SOURCE_DIR)
  target_include_directories(wato
  PRIVATE
    ${glfw3_SOURCE_DIR}/include
  )
endif()

if(DEFINED entt_SOURCE_DIR)
  target_include_directories(wato
  PRIVATE
    ${entt_SOURCE_DIR}/src
  )
endif()

if(WIN32)
  target_include_directories(wato
  PRIVATE
    deps/bx/include/compat/msvc
  )
  target_link_directories(wato
    deps/bgfx/.build/win64_vs2022/bin
    deps/glfw/lib-vc2022
  )
elseif(UNIX)
  target_include_directories(wato
  PRIVATE
    deps/bx/include/compat/linux
  )
  target_link_directories(wato
  PRIVATE
    deps/bgfx/.build/linux64_gcc/bin
  )
endif()

target_sources(wato
  PUBLIC FILE_SET HEADERS
  BASE_DIRS src
  FILES
    src/components/camera.hpp
    src/components/color.hpp
    src/components/direction.hpp
    src/components/health.hpp
    src/components/imgui.hpp
    src/components/placement_mode.hpp
    src/components/position.hpp
    src/components/rigid_body.hpp
    src/components/rotation.hpp
    src/components/scale.hpp
    src/components/scene_object.hpp
    src/components/tile.hpp
    src/components/transform3d.hpp
    src/core/action.hpp
    src/core/cache.hpp
    src/core/model_loader.hpp
    src/core/physics.hpp
    src/core/ray.hpp
    src/core/registry.hpp
    src/core/sys.hpp
    src/core/tile.hpp
    src/input/input.hpp
    src/renderer/bgfx_utils.hpp
    src/renderer/blinn_phong_material.hpp
    src/renderer/material.hpp
    src/renderer/mesh_primitive.hpp
    src/renderer/physics.hpp
    src/renderer/plane_primitive.hpp
    src/renderer/primitive.hpp
    src/systems/systems.hpp

  PRIVATE
    deps/imgui/imgui.cpp
    deps/imgui/imgui_draw.cpp
    deps/imgui/imgui_tables.cpp
    deps/imgui/imgui_widgets.cpp
    src/core/model_loader.cpp
    src/core/ray.cpp
    src/core/registry.cpp
    src/core/sys.cpp
    src/imgui/imgui_helper.cpp
    src/input/input.cpp
    src/main.cpp
    src/renderer/bgfx_utils.cpp
    src/renderer/material.cpp
    src/systems/action.cpp
    src/systems/camera.cpp
    src/systems/imgui.cpp
    src/systems/input.cpp
    src/systems/physics.cpp
    src/systems/render.cpp
)

target_include_directories(wato PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

target_link_options(wato PRIVATE -rdynamic)

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
    reactphysics3d
  )
endif()

add_custom_target(format-sources
  COMMAND clang-format -style=file -i $<TARGET_PROPERTY:wato,SOURCES>
  $<TARGET_PROPERTY:wato,HEADER_SET>
  COMMAND_EXPAND_LISTS
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
