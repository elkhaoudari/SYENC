cmake_minimum_required(VERSION 3.16)

project(symenc
  VERSION 0.1.1
  DESCRIPTION "Reference implementation of the Symbolic Encoding Protocol v0.1.1"
  LANGUAGES CXX)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are not allowed. Use: cmake -S . -B build")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

option(SYMENC_WERROR "Treat compiler warnings as errors" ON)
option(SYMENC_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(SYMENC_BUILD_TESTS "Build the test suite" ON)

include(cmake/CompilerOptions.cmake)

add_subdirectory(src)

if(SYMENC_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
