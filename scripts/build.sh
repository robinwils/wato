#! /bin/bash

# Assign positional arguments to variables
BUILD_TYPE="${1:-Release}"
CXX_COMPILER="${2:-g++}"
C_COMPILER="${3:-gcc}"
BUILD_DIR="${3:-_build}"

cmake -B $BUILD_DIR \
	-DCMAKE_BUILD_TYPE=$BUILD_TYPE \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
	-DCMAKE_CXX_COMPILER=$CXX_COMPILER \
	-DCMAKE_C_COMPILER=$C_COMPILER && \
cmake --build $BUILD_DIR --config $BUILD_TYPE
