cmake_minimum_required(VERSION 3.10)
project(ArenaAllocator VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)

add_library(arena_lib INTERFACE
        include/arena_allocator.h
)

target_include_directories(arena_lib INTERFACE include)

add_subdirectory(examples)
add_subdirectory(tests)
add_subdirectory(benchmarks)
