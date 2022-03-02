cmake_minimum_required(VERSION 3.8)
project(MIMUW-FORK C)

set(CMAKE_CXX_STANDARD "17")
set(CMAKE_C_STANDARD "11")
set(CMAKE_C_FLAGS "-g -Wall -Wextra -Wno-sign-compare")

add_library(err src/util/err.c)
add_library(HashMap src/hash.c)
add_library(Tree src/tree.c)
add_library(path src/util/paths.c)
add_executable(example example/tree_example.c)
target_link_libraries(example Tree HashMap err path pthread)

install(TARGETS DESTINATION .)
