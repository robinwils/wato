add_executable(watod)

target_sources(watod
  PUBLIC FILE_SET HEADERS
  BASE_DIRS
    ${CMAKE_SOURCE_DIR}/src
  FILES
    ${CMAKE_SOURCE_DIR}/src/components/creep.hpp
    ${CMAKE_SOURCE_DIR}/src/core/app/game_server.hpp
    ${CMAKE_SOURCE_DIR}/src/core/net/enet_base.hpp
    ${CMAKE_SOURCE_DIR}/src/core/net/enet_server.hpp
    ${CMAKE_SOURCE_DIR}/src/input/action.hpp
    ${CMAKE_SOURCE_DIR}/src/systems/action.hpp
    ${CMAKE_SOURCE_DIR}/src/systems/creep.hpp

  PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core/app/game_server.cpp
    ${CMAKE_SOURCE_DIR}/src/core/net/enet_base.cpp
    ${CMAKE_SOURCE_DIR}/src/core/net/enet_server.cpp
    ${CMAKE_SOURCE_DIR}/src/input/action.cpp
    ${CMAKE_SOURCE_DIR}/src/systems/action.cpp
    ${CMAKE_SOURCE_DIR}/src/systems/creep.cpp
    main.cpp
)

target_compile_options(watod PRIVATE "-DDOCTEST_CONFIG_DISABLE")

target_link_libraries(watod
  watolib
  ReactPhysics3D::ReactPhysics3D
)
