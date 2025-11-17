add_executable(wato_tests)

target_compile_options(wato_tests
  PUBLIC
  $<$<CONFIG:Debug>:-ggdb -fsanitize=address,undefined>
  -fsanitize=fuzzer
  -fdiagnostics-color=always
  -ftime-trace
  -O$<IF:$<CONFIG:Debug>,0,3>
)
target_link_options(wato_tests
  PUBLIC
  $<$<CONFIG:Debug>:-fsanitize=address,undefined>
  -fsanitize=fuzzer
  -rdynamic
)

if(DEFINED doctest_SOURCE_DIR)
  target_include_directories(wato_tests

    SYSTEM PUBLIC
    ${doctest_SOURCE_DIR}/doctest
    ${entt_SOURCE_DIR}/src
  )

  include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")
  doctest_discover_tests(wato_tests TEST_PREFIX "wato.")
endif()

target_sources(wato_tests
  PUBLIC FILE_SET HEADERS
  BASE_DIRS
    .
    ${CMAKE_SOURCE_DIR}/src
  FILES
    ${CMAKE_SOURCE_DIR}/src/components/rigid_body.hpp
    ${CMAKE_SOURCE_DIR}/src/core/physics/physics.hpp
    ${CMAKE_SOURCE_DIR}/src/core/snapshot.hpp
    ${CMAKE_SOURCE_DIR}/src/core/net/net.hpp
    ${CMAKE_SOURCE_DIR}/src/core/queue/ring_buffer.hpp
    ${CMAKE_SOURCE_DIR}/src/input/action.hpp
    test.hpp
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core/physics/physics.cpp
    main.cpp
)

target_link_libraries(wato_tests
  PUBLIC
    wato_common
    bgfx::bx
    spdlog::spdlog
    ReactPhysics3D::ReactPhysics3D
    doctest
)

# add_subdirectory(fuzztest)
#
# include(GoogleTest)

# enable_testing()
# fuzztest_setup_fuzzing_flags()

add_library(wato_common_fuzz INTERFACE)

target_include_directories(wato_common_fuzz INTERFACE
  $<TARGET_PROPERTY:wato_common,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(wato_common_fuzz INTERFACE
  $<TARGET_PROPERTY:wato_common,INTERFACE_COMPILE_DEFINITIONS>
)
target_compile_features(wato_common_fuzz INTERFACE
  $<TARGET_PROPERTY:wato_common,INTERFACE_COMPILE_FEATURES>
)

get_target_property(COMMON_LIBS wato_common INTERFACE_LINK_LIBRARIES)
if(COMMON_LIBS)
  target_link_libraries(wato_common_fuzz INTERFACE ${COMMON_LIBS})
endif()

add_executable(wato_fuzz)

target_compile_definitions(wato_fuzz PUBLIC DOCTEST_CONFIG_DISABLE)
target_compile_definitions(wato_fuzz PUBLIC FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
target_compile_definitions(wato_fuzz PUBLIC BX_CONFIG_DEBUG)

target_sources(wato_fuzz
  PUBLIC FILE_SET HEADERS
  BASE_DIRS
    .
    ${CMAKE_SOURCE_DIR}/src
  FILES
    ${CMAKE_SOURCE_DIR}/src/core/snapshot.hpp
    test.hpp
  PRIVATE
    main_fuzz.cpp
)

target_link_libraries(wato_fuzz
  PUBLIC
    wato_common_fuzz
    bgfx::bx
    spdlog::spdlog
    ReactPhysics3D::ReactPhysics3D
)

target_compile_options(wato_fuzz
  PUBLIC
  -g -UNDEBUG
  -fsanitize-coverage=inline-8bit-counters -fsanitize-coverage=trace-cmp
  -fsanitize=undefined,address -DADDRESS_SANITIZER
  -fdiagnostics-color=always
  -fsanitize=fuzzer
  -ftime-trace
  -O1
)
target_link_options(wato_fuzz
  PUBLIC
  -fsanitize-coverage=inline-8bit-counters -fsanitize-coverage=trace-cmp
  -fsanitize=undefined,address
  -fsanitize=fuzzer
  -rdynamic
)

# link_fuzztest(wato_fuzz)
# gtest_discover_tests(wato_fuzz)
  # doctest_discover_tests(wato_fuzz TEST_PREFIX "wato.")
