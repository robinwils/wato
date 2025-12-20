cmake_minimum_required(VERSION 3.29)

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

option(ENABLE_FUZZING "Build with libfuzzer support (wato_fuzz)" OFF)

option(ENABLE_TRACING "Enable clang time trace" OFF)

option(ENABLE_CLIENT "Enable client (wato)" ON)

option(ENABLE_SERVER "Enable server (watod)" ON)

option(ENABLE_TESTS "Enable tests (wato_tests)" ON)

set(LIB_FUZZING_ENGINE "-fsanitize=fuzzer,undefined,address" CACHE STRING
  "optional fuzzing engine library"
)

set(TF_BUILD_TESTS OFF CACHE BOOL "Disable taskflow test builds" FORCE)
set(TF_BUILD_EXAMPLES OFF CACHE BOOL "Disable taskflow example builds" FORCE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_MAKEFILE ON)

include(FetchContent)
FetchContent_Declare(
  entt
  GIT_REPOSITORY https://github.com/skypjack/entt
  GIT_TAG v3.15.0
  FIND_PACKAGE_ARGS NAMES entt
)
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm
  GIT_TAG 0.9.9.8
  FIND_PACKAGE_ARGS NAMES glm
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
  GIT_TAG v3.10.0
  FIND_PACKAGE_ARGS NAMES taskflow
)

if (ENABLE_TESTS)
  FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest
    GIT_TAG v2.4.11
    FIND_PACKAGE_ARGS NAMES doctest
  )
endif()

set(FETCHED_DEPS_NAMES
  entt
  glm
  enet
  argh
  taskflow
  doctest
)

FetchContent_MakeAvailable(${FETCHED_DEPS_NAMES})
set(OTHER_DEPS
  ReactPhysics3D
  spdlog
  Sodium
  bgfx
)

if (ENABLE_CLIENT)
  list(APPEND OTHER_DEPS glfw3 imgui assimp)

  if(APPLE)
    find_library(METAL Metal REQUIRED)
    if (NOT METAL)
      message(FATAL_ERROR "Metal not found")
    endif()
  elseif(LINUX)
    list(APPEND OTHER_DEPS X11 Vulkan)
  elseif(WIN32)
    list(APPEND OTHER_DEPS Vulkan)
  endif()
endif()

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

find_path(SAFEINT_INCLUDE_DIRS "SafeInt.hpp" REQUIRED)

include(CTest)

add_library(wato_common INTERFACE)
if(BUILD_SHARED_LIBS)
  add_library(watolib SHARED)
else()
  add_library(watolib STATIC)
endif()

# to get libwato.so not libwatolib.so, but on mscv this creates
# a linker error because exe and lib are the same name
if(NOT MSVC)
  set_property(TARGET watolib PROPERTY OUTPUT_NAME wato)
endif()

if (ENABLE_CLIENT)
  add_executable(wato)
  target_compile_definitions(wato PUBLIC DOCTEST_CONFIG_DISABLE)
  target_include_directories(wato
    PRIVATE
      src
      include
    SYSTEM PRIVATE
      src/imgui
  )
  target_sources(wato
    PUBLIC FILE_SET HEADERS
    BASE_DIRS src
    FILES
    src/core/app/game_client.hpp
    src/core/tile.hpp
    src/core/window.hpp
    src/imgui/wato_imgui_config.h
    src/imgui/imgui_helper.h
    src/input/action.hpp
    src/input/input.hpp
    src/registry/game_registry.hpp
    src/renderer/animation.hpp
    src/renderer/bgfx_utils.hpp
    src/renderer/blinn_phong_material.hpp
    src/renderer/grid_preview_material.hpp
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
    src/core/app/game_client.cpp
    src/core/physics/event_handler.cpp
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

    shaders/fs_blinnphong.sc
    shaders/fs_grid.sc
    shaders/fs_simple.sc
    shaders/vs_blinnphong.sc
    shaders/vs_blinnphong_skinned.sc
    shaders/vs_grid.sc
    shaders/vs_simple.sc
  )
  target_include_directories(wato PRIVATE ${CMAKE_BINARY_DIR}/include/generated/shaders)
  target_link_libraries(wato
    watolib
    glfw
    assimp::assimp
    bgfx::bgfx
    bgfx::bimg_decode
    bgfx::bimg
    imgui::imgui
  )

  if(LINUX)
    target_link_libraries(wato
      X11
      Vulkan::Vulkan
    )
  elseif(APPLE)
    target_link_libraries(wato
      # OpenGL::GL
      # OpenGL::GLU
      ${METAL}
    )
  elseif(WIN32)
    target_link_libraries(wato
      Vulkan::Vulkan
      # OpenGL::GL
      # OpenGL::GLU
    )
  endif()

endif()

if (ENABLE_SERVER)
  add_subdirectory(src/watod)
endif()

if (ENABLE_TESTS)
  add_subdirectory(test)
endif()

# Preprocessor
target_compile_definitions(wato_common
  INTERFACE
    SPDLOG_ACTIVE_LEVEL=$<IF:$<CONFIG:Debug>,SPDLOG_LEVEL_TRACE,SPDLOG_LEVEL_INFO>
    WATO_DEBUG=$<IF:$<CONFIG:Debug>,1,0>
    GLM_FORCE_SIZE_T_LENGTH
    GLM_ENABLE_EXPERIMENTAL
)

target_compile_definitions(watolib PUBLIC DOCTEST_CONFIG_DISABLE)

if(UNIX)
  target_link_options(wato_common INTERFACE -rdynamic)
elseif(WIN32)
  target_compile_definitions(wato_common INTERFACE NOMINMAX)
endif()

if (MSVC)
  target_compile_options(wato_common
    INTERFACE
      $<$<CONFIG:Debug>:/fsanitize=address>
  )
  target_compile_definitions(wato_common
    INTERFACE
      $<$<CONFIG:Debug>:_DISABLE_STRING_ANNOTATION _DISABLE_VECTOR_ANNOTATION>)

  target_link_options(wato_common
    INTERFACE
      $<$<CONFIG:Debug>:/fsanitize=address>
  )
else()
  target_compile_options(wato_common
    INTERFACE
      $<$<CONFIG:Debug>:-O0 -ggdb -fsanitize=address,undefined>
      -fdiagnostics-color=always
  )
  target_link_options(wato_common
    INTERFACE
      $<$<CONFIG:Debug>:-fsanitize=address,undefined>
      -rdynamic
  )
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_options(wato_common
    INTERFACE
      $<$<CONFIG:Debug>:-static-libasan -static-libubsan>
  )
elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" AND ENABLE_TRACING)
  target_compile_options(wato_common INTERFACE -ftime-trace)
endif()


include(cmake/CompilerWarnings.cmake)
wato_set_project_warnings(wato_common
  VISIBILITY "INTERFACE"
  AS_ERRRORS ${wato_WARNINGS_AS_ERRORS})

target_compile_features(wato_common INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

target_include_directories(wato_common INTERFACE ${SAFEINT_INCLUDE_DIRS})

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

bgfx_compile_shaders(
  TYPE VERTEX
  SHADERS
    shaders/vs_simple.sc
    shaders/vs_grid.sc
    shaders/vs_blinnphong.sc
    shaders/vs_blinnphong_skinned.sc
  VARYING_DEF ${CMAKE_SOURCE_DIR}/shaders/varying.def.sc
  OUTPUT_DIR ${CMAKE_BINARY_DIR}/include/generated/shaders
  AS_HEADERS
)

bgfx_compile_shaders(
  TYPE FRAGMENT
  SHADERS
    shaders/fs_simple.sc
    shaders/fs_grid.sc
    shaders/fs_blinnphong.sc
  VARYING_DEF ${CMAKE_SOURCE_DIR}/shaders/varying.def.sc
  OUTPUT_DIR ${CMAKE_BINARY_DIR}/include/generated/shaders
  AS_HEADERS
)

target_sources(watolib
  PUBLIC FILE_SET HEADERS
  BASE_DIRS src
  FILES
    src/components/animator.hpp
    src/components/camera.hpp
    src/components/color.hpp
    src/components/creep.hpp
    src/components/direction.hpp
    src/components/health.hpp
    src/components/imgui.hpp
    src/components/path.hpp
    src/components/placement_mode.hpp
    src/components/net.hpp
    src/components/rigid_body.hpp
    src/components/scene_object.hpp
    src/components/tile.hpp
    src/components/transform3d.hpp
    src/components/velocity.hpp
    src/core/app/app.hpp
    src/core/app/game_server.hpp
    src/core/graph.hpp
    src/core/net/enet_base.hpp
    src/core/net/enet_client.hpp
    src/core/net/enet_server.hpp
    src/core/physics/event_handler.hpp
    src/core/physics/physics.hpp
    src/core/queue/channel.hpp
    src/core/queue/ring_buffer.hpp
    src/core/snapshot.hpp
    src/core/state.hpp
    src/core/sys/backtrace.hpp
    src/core/sys/log.hpp
    src/core/tower_building_handler.hpp
    src/registry/registry.hpp
    src/systems/ai.hpp
    src/systems/physics.hpp
    src/systems/rigid_bodies_update.hpp
    src/systems/system.hpp
    src/systems/tower_built.hpp

  PRIVATE
    src/core/app/app.cpp
    src/core/app/game_server.cpp
    src/core/graph.cpp
    src/core/net/enet_base.cpp
    src/core/net/enet_client.cpp
    src/core/net/enet_server.cpp
    src/core/physics/event_handler.cpp
    src/core/physics/physics.cpp
    src/core/sys/backtrace.cpp
    src/core/tower_building_handler.cpp
    src/systems/ai.cpp
    src/systems/physics.cpp
    src/systems/rigid_bodies_update.cpp
    src/systems/tower_built.cpp
)

target_include_directories(wato_common INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")

# Link libraries
target_link_libraries(watolib
  PUBLIC
    wato_common
    bgfx::bx
    sodium
    spdlog::spdlog
    ReactPhysics3D::ReactPhysics3D
)
