add_executable(watod)

target_sources(watod
  PUBLIC FILE_SET HEADERS
  BASE_DIRS
    ${CMAKE_SOURCE_DIR}/src
  FILES
    ${CMAKE_SOURCE_DIR}/src/core/app/game_server.hpp
    ${CMAKE_SOURCE_DIR}/src/core/net/enet_server.hpp
    ${CMAKE_SOURCE_DIR}/src/systems/creep.hpp

  PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core/app/game_server.cpp
    ${CMAKE_SOURCE_DIR}/src/core/net/enet_server.cpp
    ${CMAKE_SOURCE_DIR}/src/systems/creep.cpp
    main.cpp
)

target_compile_options(watod PRIVATE "-DDOCTEST_CONFIG_DISABLE")

target_link_libraries(watod
  watolib
  reactphysics3d
)
