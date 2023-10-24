cmake_minimum_required(VERSION 2.6)

project(filescanner)

include_directories(${PROJECT_SOURCE_DIR}/src/)
include_directories(${PROJECT_SOURCE_DIR}/src/digest)
include_directories(${PROJECT_SOURCE_DIR}/src/filescanner)

file(GLOB_RECURSE SRC_DIR_LIST "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_compile_options(-DUNICODE) 

add_executable(Test ${SRC_DIR_LIST})

