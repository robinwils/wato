project(WaTo)

cmake_minimum_required(VERSION 3.31)

include(FetchContent)
FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG v5.4.3
  PREFIX deps/assimp
)

FetchContent_MakeAvailable(assimp)
