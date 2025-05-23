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
find_package(ZLIB REQUIRED)

find_program(CLANG_TIDY_BIN clang-tidy REQUIRED)

option(DEBUG_ENABLE "Enable debug features" OFF)

option(wato_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

set(TF_BUILD_TESTS OFF CACHE BOOL "Disable taskflow test builds" FORCE)
set(TF_BUILD_EXAMPLES OFF CACHE BOOL "Disable taskflow example builds" FORCE)

include(FetchContent)
FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG v5.4.3
  FIND_PACKAGE_ARGS NAMES assimp REQUIRED
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

find_package(Sodium REQUIRED)

set(DEPENDENCY_NAMES assimp
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

FetchContent_MakeAvailable(${DEPENDENCY_NAMES})
# Display all fetched content info
message(STATUS "Fetched dependencies information:")
message(STATUS "----------------------------------")

# For each dependency, show key paths
foreach(dep ${DEPENDENCY_NAMES} Sodium)
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
add_executable(wato)
add_subdirectory(src/watod)
add_subdirectory(test)

target_compile_options(watolib PRIVATE "-DDOCTEST_CONFIG_DISABLE")
target_compile_options(wato PRIVATE "-DDOCTEST_CONFIG_DISABLE")
if(DEBUG_ENABLE)
  target_compile_definitions(wato_common INTERFACE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE BX_CONFIG_DEBUG)
  set(WATO_DEBUG 1)
else()
  set(WATO_DEBUG 0)
endif()

# Configure the file using the provided (or default) value
configure_file(config.h.in config.h @ONLY)


include(cmake/CompilerWarnings.cmake)
wato_set_project_warnings(wato_common
  VISIBILITY "INTERFACE"
  AS_ERRRORS ${wato_WARNINGS_AS_ERRORS})

target_compile_features(wato_common INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

target_include_directories(wato_common
  SYSTEM INTERFACE
    deps/bimg/include
    deps/bx/include
)

target_include_directories(wato
  PRIVATE
    src
    src/imgui
)

target_include_directories(wato
  SYSTEM PRIVATE
    deps/bgfx/include
    deps/imgui
    src/imgui
    deps/iconfontcppheaders
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

if(WIN32)
  target_include_directories(wato_common
  SYSTEM INTERFACE
    deps/bx/include/compat/msvc
  )
  target_link_directories(wato_common
  INTERFACE
    deps/bgfx/.build/win64_vs2022/bin
    deps/glfw/lib-vc2022
  )
elseif(UNIX)
  target_include_directories(wato_common
  SYSTEM INTERFACE
    deps/bx/include/compat/linux
  )
  target_link_directories(wato_common
  INTERFACE
    deps/bgfx/.build/linux64_gcc/bin
  )
endif()


target_sources(watolib
  PUBLIC FILE_SET HEADERS
  BASE_DIRS src
  FILES
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
    src/core/net/enet_base.hpp
    src/core/physics.hpp
    src/core/queue/ring_buffer.hpp
    src/core/snapshot.hpp
    src/core/sys/backtrace.hpp
    src/core/sys/log.hpp
    src/registry/registry.hpp
    src/systems/physics.hpp
    src/systems/system.hpp

  PRIVATE
    src/core/app/app.cpp
    src/core/event_handler.cpp
    src/core/net/enet_base.cpp
    src/core/physics.cpp
    src/core/snapshot.cpp
    src/core/sys/backtrace.cpp
    src/systems/physics.cpp
)

target_sources(wato
  PUBLIC FILE_SET HEADERS
  BASE_DIRS src
  FILES
    src/core/app/game_client.hpp
    src/core/cache.hpp
    src/core/model_loader.hpp
    src/core/net/enet_client.hpp
    src/core/tile.hpp
    src/core/window.hpp
    src/input/action.hpp
    src/input/input.hpp
    src/registry/game_registry.hpp
    src/renderer/bgfx_utils.hpp
    src/renderer/blinn_phong_material.hpp
    src/renderer/material.hpp
    src/renderer/mesh_primitive.hpp
    src/renderer/plane_primitive.hpp
    src/renderer/primitive.hpp
    src/renderer/renderer.hpp
    src/systems/action.hpp
    src/systems/input.hpp
    src/systems/render.hpp
    src/systems/sync.hpp

  PRIVATE
    deps/imgui/imgui.cpp
    deps/imgui/imgui_draw.cpp
    deps/imgui/imgui_tables.cpp
    deps/imgui/imgui_widgets.cpp
    src/core/app/game_client.cpp
    src/core/model_loader.cpp
    src/core/net/enet_client.cpp
    src/core/sys/mem.cpp
    src/core/window.cpp
    src/imgui/imgui_helper.cpp
    src/input/action.cpp
    src/input/input.cpp
    src/main.cpp
    src/registry/game_registry.cpp
    src/renderer/bgfx_utils.cpp
    src/renderer/material.cpp
    src/renderer/renderer.cpp
    src/systems/action.cpp
    src/systems/input.cpp
    src/systems/render.cpp
    src/systems/sync.cpp
)

target_include_directories(wato_common INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")

target_link_options(wato_common INTERFACE -rdynamic)

# Link libraries
target_link_libraries(watolib
  wato_common
  bxDebug
  bimg_decodeDebug
  bimg_encodeDebug
  bimgDebug
  reactphysics3d
  spdlog
  sodium
)

target_link_libraries(wato watolib bgfxDebug)

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
