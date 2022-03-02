cmake_minimum_required(VERSION 3.8)
project(MIMUW-FORK C)

set(CMAKE_CXX_STANDARD "17")
set(CMAKE_C_STANDARD "11")
set(CMAKE_C_FLAGS "-g -Wall -Wextra -Wno-sign-compare")

add_library(hash src/hash.c)
add_library(tree src/tree.c)
add_library(err src/util/err.c)
add_library(paths src/util/paths.c)
set(SOURCE tree hash paths err pthread)

add_executable(example example/tree_example.c)
add_executable(tree_test test/tree_test.c)
add_executable(hash_test test/hash_test.c)
add_executable(paths_test test/paths_test.c)

target_link_libraries(example ${SOURCE})
target_link_libraries(tree_test ${SOURCE})
target_link_libraries(hash_test ${SOURCE})
target_link_libraries(paths_test ${SOURCE})

install(TARGETS DESTINATION .)
