#!/bin/bash -eu

# see example on https://github.com/google/oss-fuzz/blob/master/projects/boringssl/build.sh
# build project
cmake --preset unixlike-clang-debug -G "Ninja" -B tmp\
    -DCMAKE_CXX_COMPILER=$CXX \
    -DCMAKE_CXX_FLAGS="$CXXFLAGS" \
    -DENABLE_FUZZING=ON \
    -DLIB_FUZZING_ENGINE=$LIB_FUZZING_ENGINE
cmake --build tmp --parallel --target wato_fuzz

cp $SRC/wato/tmp/test/wato_fuzz $OUT/
if [ -d "$SRC/wato/corpus" ]; then
    zip -j $OUT/wato_fuzz_seed_corpus.zip $SRC/wato/corpus/*
fi
