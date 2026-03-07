option(ENABLE_COVERAGE "Enable code coverage instrumentation (Clang only)" OFF)

if(ENABLE_COVERAGE)
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    message(FATAL_ERROR "ENABLE_COVERAGE requires Clang")
  endif()

  find_program(GRCOV grcov REQUIRED)
  find_program(LLVM_PROFDATA llvm-profdata HINTS /usr/lib/llvm-19/bin REQUIRED)

  set(COVERAGE_FLAGS -fprofile-instr-generate -fcoverage-mapping)
  set(COVERAGE_PROFRAW_DIR "${CMAKE_BINARY_DIR}/profraw")
  set(COVERAGE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/coverage")
  set(COVERAGE_PROFDATA "${CMAKE_BINARY_DIR}/coverage.profdata")

  # Apply coverage flags globally via the wato_common interface
  target_compile_options(wato_common INTERFACE ${COVERAGE_FLAGS})
  target_link_options(wato_common INTERFACE ${COVERAGE_FLAGS})

  # Custom target: run tests, merge profraw, run grcov
  add_custom_target(coverage
    COMMENT "Running tests and generating coverage report"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${COVERAGE_PROFRAW_DIR}" "${COVERAGE_OUTPUT_DIR}" "${COVERAGE_PROFDATA}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${COVERAGE_PROFRAW_DIR}"
    COMMAND ${CMAKE_COMMAND} -E env
      "LLVM_PROFILE_FILE=${COVERAGE_PROFRAW_DIR}/wato_%p_%m.profraw"
      ${CMAKE_CTEST_COMMAND} --test-dir "${CMAKE_BINARY_DIR}" --output-on-failure
    COMMAND ${LLVM_PROFDATA} merge -sparse
      ${COVERAGE_PROFRAW_DIR}
      -o "${COVERAGE_PROFDATA}"
    COMMAND ${GRCOV} "${COVERAGE_PROFRAW_DIR}"
      -s "${CMAKE_SOURCE_DIR}"
      --binary-path "${CMAKE_BINARY_DIR}"
      --llvm-path "$<PATH:GET_PARENT_PATH,${LLVM_PROFDATA}>"
      -t html
      --branch
      --ignore-not-existing
      --keep-only "src/*"
      -o "${COVERAGE_OUTPUT_DIR}"
    DEPENDS wato_tests
    USES_TERMINAL
    VERBATIM
  )

  message(STATUS "Coverage enabled: report target 'coverage' writes to ${COVERAGE_OUTPUT_DIR}")
endif()
