add_executable(wato_tests)

if(DEFINED doctest_SOURCE_DIR)
  target_include_directories(wato_tests
    SYSTEM PUBLIC
    ${doctest_SOURCE_DIR}
  )

  include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")
  doctest_discover_tests(wato_tests TEST_PREFIX "wato.")
  target_link_libraries(wato_tests doctest)
endif()

target_sources(wato_tests
  PUBLIC FILE_SET HEADERS
  BASE_DIRS
    ${CMAKE_SOURCE_DIR}/src
  FILES
  PRIVATE
    main.cpp
)

target_link_libraries(wato_tests
  watolib
)

