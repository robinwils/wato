add_executable(watod
  main.cpp
)

if (DEBUG_ENABLE)
  target_compile_options(watod PUBLIC ${DEBUG_FLAGS})
else()
  target_compile_options(watod PUBLIC ${RELEASE_FLAGS})
endif()

target_include_directories(watod
  PRIVATE
    ${PROJECT_SOURCE_DIR}/deps/bx/include
    ${PROJECT_SOURCE_DIR}/src
    ${enet_SOURCE_DIR}/include
)

target_include_directories(watod PRIVATE "${CMAKE_BINARY_DIR}")

if(DEFINED entt_SOURCE_DIR)
  target_include_directories(watod
  PRIVATE
    ${entt_SOURCE_DIR}/src
  )
endif()

if(DEFINED reactphysics3d_SOURCE_DIR)
  target_include_directories(watod
  PRIVATE
    ${reactphysics3d_SOURCE_DIR}/include
  )
endif()

target_link_libraries(watod
  enet
  reactphysics3d
  # Add other server-side libs
)

target_compile_features(watod INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

include(${PROJECT_SOURCE_DIR}/cmake/CompilerWarnings.cmake)
wato_set_project_warnings(
  watod
  ${wato_WARNINGS_AS_ERRORS}
  ""
  ""
  ""
  ""
)

