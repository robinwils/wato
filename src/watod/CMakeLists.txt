add_executable(watod)

target_sources(watod
  PUBLIC FILE_SET HEADERS
  BASE_DIRS
    ${CMAKE_SOURCE_DIR}/src
  FILES
    ${CMAKE_SOURCE_DIR}/src/core/app/game_server.hpp
    ${CMAKE_SOURCE_DIR}/src/core/net/enet_server.hpp

  PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core/app/game_server.cpp
    ${CMAKE_SOURCE_DIR}/src/core/net/enet_server.cpp
    main.cpp
)

target_link_libraries(watod
  watolib
  reactphysics3d
)
