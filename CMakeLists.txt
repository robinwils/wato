cmake_minimum_required(VERSION 3.31)

project(
  WaTo
  VERSION 0.0.1
  DESCRIPTION "3D PvP Tower Defense game"
  HOMEPAGE_URL "https://github.com/robinwils/wato"
  LANGUAGES CXX C
)

if (NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif()

set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_program(CLANG_TIDY_BIN clang-tidy)

option(wato_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

set(TF_BUILD_TESTS OFF CACHE BOOL "Disable taskflow test builds" FORCE)
set(TF_BUILD_EXAMPLES OFF CACHE BOOL "Disable taskflow example builds" FORCE)

set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_MAKEFILE ON)

include(FetchContent)
FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG v6.0.2
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
FetchContent_Declare(
  enet
  GIT_REPOSITORY https://github.com/zpl-c/enet
  GIT_TAG v2.6.2
  FIND_PACKAGE_ARGS NAMES enet
)

FetchContent_Declare(
  argh
  GIT_REPOSITORY https://github.com/adishavit/argh.git
  GIT_TAG v1.3.2
  FIND_PACKAGE_ARGS NAMES argh
)

FetchContent_Declare(
  taskflow
  GIT_REPOSITORY https://github.com/taskflow/taskflow.git
  GIT_TAG v3.9.0
  FIND_PACKAGE_ARGS NAMES taskflow
)

FetchContent_Declare(
  doctest
  GIT_REPOSITORY https://github.com/doctest/doctest
  GIT_TAG v2.4.11
  FIND_PACKAGE_ARGS NAMES doctest
)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog
  GIT_TAG v1.15.3
  FIND_PACKAGE_ARGS NAMES spdlog
)

set(FETCHED_DEPS_NAMES
  assimp
  glfw3
  entt
  glm
  reactphysics3d
  enet
  argh
  taskflow
  doctest
  spdlog
)

FetchContent_MakeAvailable(${FETCHED_DEPS_NAMES})
set(OTHER_DEPS
  Sodium
  bgfx
)

foreach(dep ${OTHER_DEPS})
  find_package(${dep} REQUIRED)
endforeach()

# vcpkg loses the dependency of bx on windows which fails to pull in all headers
# credit https://github.com/openblack/openblack
find_package(bgfx REQUIRED)
if (TARGET bgfx::bgfx AND TARGET bgfx::bx)
  target_link_libraries(bgfx::bgfx INTERFACE bgfx::bx)
endif ()

# Display all fetched content info
message(STATUS "Fetched dependencies information:")
message(STATUS "----------------------------------")

# For each dependency, show key paths
foreach(dep ${FETCHED_DEPS_NAMES} ${OTHER_DEPS})
  if(${dep}_POPULATED OR ${dep}_FOUND)
    if(${dep}_SOURCE_DIR)  # Was fetched and built
      message(STATUS "Dependency: ${dep} fetched and built")
      message(STATUS "  Source dir: ${${dep}_SOURCE_DIR}")
      message(STATUS "  Binary dir: ${${dep}_BINARY_DIR}")
      message(STATUS "  Populated: ${${dep}_POPULATED}")
      message(STATUS "  Subbuild dir: ${${dep}_SUBBUILD_DIR}")
    else()
      message(STATUS "Dependency: ${dep} found on system")

      # Try to get common package variables
      foreach(var IN ITEMS FOUND VERSION DIR INCLUDE_DIR INCLUDE_DIRS LIBRARIES LIBRARY_DIR LIBRARIES_RELEASE LIBS DEFINITIONS)
        if(DEFINED ${dep}_${var})
          message(STATUS "  ${var}: ${${dep}_${var}}")
        endif()
      endforeach()
    endif()
  else()
    message(FATAL_ERROR "${dep} was not found!")
  endif()
endforeach()

include(CTest)

add_library(wato_common INTERFACE)
if(BUILD_SHARED_LIBS)
  add_library(watolib SHARED)
else()
  add_library(watolib STATIC)
endif()

# to get libwato.so not libwatolib.so
set_property(TARGET watolib PROPERTY OUTPUT_NAME wato)
add_executable(wato)
add_subdirectory(src/watod)
add_subdirectory(test)

# Preprocessor
target_compile_definitions(wato_common
  INTERFACE
    SPDLOG_ACTIVE_LEVEL=$<IF:$<CONFIG:Debug>,SPDLOG_LEVEL_TRACE,SPDLOG_LEVEL_INFO>
    WATO_DEBUG=$<IF:$<CONFIG:Debug>,1,0>
    GLM_FORCE_SIZE_T_LENGTH
    DOCTEST_CONFIG_DISABLE
)

if(UNIX)
  target_compile_options(wato_common
    INTERFACE
      $<$<CONFIG:Debug>:-ggdb -fsanitize=address,undefined>
      -fdiagnostics-color=always
      -O$<IF:$<CONFIG:Debug>,0,3>
  )
  target_link_options(wato_common
    INTERFACE
      $<$<CONFIG:Debug>:-fsanitize=address,undefined>
  )
endif()

include(cmake/CompilerWarnings.cmake)
wato_set_project_warnings(wato_common
  VISIBILITY "INTERFACE"
  AS_ERRRORS ${wato_WARNINGS_AS_ERRORS})

target_compile_features(wato_common INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

target_include_directories(wato
  PRIVATE
    src
    src/imgui
    include
)

target_include_directories(wato
  SYSTEM PRIVATE
    deps/imgui
    src/imgui
)

if(DEFINED glfw3_SOURCE_DIR)
  target_include_directories(wato
  SYSTEM PUBLIC
    ${glfw3_SOURCE_DIR}/include
  )
endif()

if(DEFINED glm_SOURCE_DIR)
  target_include_directories(wato_common
  SYSTEM INTERFACE
    ${glm_SOURCE_DIR}
  )
endif()

if(DEFINED entt_SOURCE_DIR)
  target_include_directories(wato_common
    SYSTEM INTERFACE
    ${entt_SOURCE_DIR}/src
  )
endif()

if(DEFINED reactphysics3d_SOURCE_DIR)
  target_include_directories(wato_common
  SYSTEM INTERFACE
    ${reactphysics3d_SOURCE_DIR}/include
  )
endif()

if(DEFINED enet_SOURCE_DIR)
  target_include_directories(wato_common
  SYSTEM INTERFACE
    ${enet_SOURCE_DIR}/include
  )
endif()

if(DEFINED argh_SOURCE_DIR)
  target_include_directories(wato_common
  SYSTEM INTERFACE
    ${argh_SOURCE_DIR}
  )
endif()

if(DEFINED taskflow_SOURCE_DIR)
  target_include_directories(wato_common
  SYSTEM INTERFACE
    ${taskflow_SOURCE_DIR}
  )
endif()

if(DEFINED spdlog_SOURCE_DIR)
  target_include_directories(wato_common
  SYSTEM INTERFACE
    ${spdlog_SOURCE_DIR}/include
  )
endif()

target_sources(watolib
  PUBLIC FILE_SET HEADERS
  BASE_DIRS src
  FILES
    src/components/animator.hpp
    src/components/camera.hpp
    src/components/color.hpp
    src/components/direction.hpp
    src/components/health.hpp
    src/components/imgui.hpp
    src/components/placement_mode.hpp
    src/components/rigid_body.hpp
    src/components/scene_object.hpp
    src/components/tile.hpp
    src/components/transform3d.hpp
    src/core/app/app.hpp
    src/core/event_handler.hpp
    src/core/physics.hpp
    src/core/queue/ring_buffer.hpp
    src/core/snapshot.hpp
    src/core/sys/backtrace.hpp
    src/core/sys/log.hpp
    src/core/tower_building_handler.hpp
    src/registry/registry.hpp
    src/systems/physics.hpp
    src/systems/system.hpp

  PRIVATE
    src/core/app/app.cpp
    src/core/event_handler.cpp
    src/core/physics.cpp
    src/core/snapshot.cpp
    src/core/sys/backtrace.cpp
    src/core/tower_building_handler.cpp
    src/systems/physics.cpp
)

target_sources(wato
  PUBLIC FILE_SET HEADERS
  BASE_DIRS src
  FILES
    src/core/app/game_client.hpp
    src/core/event_handler.hpp
    src/core/net/enet_base.hpp
    src/core/net/enet_client.hpp
    src/core/tile.hpp
    src/core/window.hpp
    src/input/action.hpp
    src/input/input.hpp
    src/registry/game_registry.hpp
    src/renderer/animation.hpp
    src/renderer/bgfx_utils.hpp
    src/renderer/blinn_phong_material.hpp
    src/renderer/material.hpp
    src/renderer/model.hpp
    src/renderer/plane_primitive.hpp
    src/renderer/primitive.hpp
    src/renderer/renderer.hpp
    src/renderer/skeleton.hpp
    src/renderer/vertex_layout.hpp
    src/resource/asset.hpp
    src/resource/cache.hpp
    src/resource/model_loader.hpp
    src/resource/shader_loader.hpp
    src/resource/texture_loader.hpp
    src/systems/action.hpp
    src/systems/animation.hpp
    src/systems/input.hpp
    src/systems/render.hpp
    src/systems/sync.hpp

  PRIVATE
    deps/imgui/imgui.cpp
    deps/imgui/imgui_draw.cpp
    deps/imgui/imgui_tables.cpp
    deps/imgui/imgui_widgets.cpp
    src/core/app/game_client.cpp
    src/core/event_handler.cpp
    src/core/net/enet_base.cpp
    src/core/net/enet_client.cpp
    src/core/sys/mem.cpp
    src/core/window.cpp
    src/imgui/imgui_helper.cpp
    src/input/action.cpp
    src/input/input.cpp
    src/main.cpp
    src/registry/game_registry.cpp
    src/renderer/bgfx_utils.cpp
    src/renderer/model.cpp
    src/renderer/renderer.cpp
    src/renderer/skeleton.cpp
    src/resource/asset.cpp
    src/resource/model_loader.cpp
    src/resource/shader_loader.cpp
    src/resource/texture_loader.cpp
    src/systems/action.cpp
    src/systems/animation.cpp
    src/systems/input.cpp
    src/systems/render.cpp
    src/systems/sync.cpp
)

target_include_directories(wato_common INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")

target_link_options(wato_common INTERFACE -rdynamic)

# Link libraries
target_link_libraries(watolib
  PUBLIC
    wato_common
    bgfx::bx
    sodium
    spdlog
    reactphysics3d
)

target_link_libraries(wato
  watolib
  glfw
  assimp
  bgfx::bgfx
  bgfx::bimg_decode
  bgfx::bimg
)

if(LINUX)
  target_link_libraries(wato
    X11
    vulkan
    GL
    GLU
  )
elseif(APPLE)
  target_link_libraries(wato
    GL
    GLU
  )
endif()

set(wato_includes "$<TARGET_PROPERTY:wato,INCLUDE_DIRECTORIES>")
set(wato_sources "$<FILTER:$<TARGET_PROPERTY:wato,SOURCES>,INCLUDE,src/*>")
set(wato_headers "$<FILTER:$<TARGET_PROPERTY:wato,HEADER_SET>,INCLUDE,src/*>")
set(wato_opts "$<TARGET_PROPERTY:wato,COMPILE_OPTIONS>")

add_custom_target(format-sources
  COMMAND clang-format -style=file -i ${wato_sources} ${wato_headers}
  # COMMAND_EXPAND_LISTS option means that paths with spaces will be handled correctly.
  # see https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html
  COMMAND_EXPAND_LISTS
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_custom_target(tidy-sources
  COMMAND ${CLANG_TIDY_BIN} --fix --format-style=file
    -p ${CMAKE_BINARY_DIR}
    ${wato_sources}
    ${wato_headers}
    --
    ${compile_opts}
    "$<$<BOOL:${wato_includes}>:-I$<JOIN:${wato_includes},;-I>>"
  COMMAND_EXPAND_LISTS
  VERBATIM
  COMMENT "Running clang-tidy with auto-fix on all sources..."
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
