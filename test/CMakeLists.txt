add_executable(wato_tests)

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

