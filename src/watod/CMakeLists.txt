add_executable(watod)


if(DEFINED entt_SOURCE_DIR)
  target_include_directories(watod
  SYSTEM PUBLIC
    ${entt_SOURCE_DIR}/src
  )
endif()

if(DEFINED reactphysics3d_SOURCE_DIR)
  target_include_directories(watod
  SYSTEM PUBLIC
    ${reactphysics3d_SOURCE_DIR}/include
  )
endif()

target_sources(watod
  PUBLIC FILE_SET HEADERS
  BASE_DIRS
    ${CMAKE_SOURCE_DIR}/src
  FILES
    ${CMAKE_SOURCE_DIR}/src/core/app/server.hpp
    ${CMAKE_SOURCE_DIR}/src/core/app/server.hpp

  PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core/app/server.cpp
    main.cpp
)

target_link_libraries(watod
  watolib
  reactphysics3d
)

target_compile_features(watod INTERFACE cxx_std_${CMAKE_CXX_STANDARD})


