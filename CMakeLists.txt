cmake_minimum_required(VERSION 3.10)
project(mydbg LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to a debug build (no optimization, symbols on)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# The debugger itself
add_executable(mydbg
  src/main.cpp
  src/debugger.cpp
  src/breakpoint.cpp
  src/command.cpp
)
target_compile_options(mydbg PRIVATE -Wall -Wextra)

# Sample program to debug. Non-PIE so addresses are fixed.
add_executable(target tests/target.c)
target_compile_options(target PRIVATE -g -O0)
target_link_options(target PRIVATE -no-pie)
set_target_properties(target PROPERTIES POSITION_INDEPENDENT_CODE OFF)