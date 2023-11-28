cmake_minimum_required(VERSION 2.6)

project(filescanner)

include_directories(${PROJECT_SOURCE_DIR}/src/)
include_directories(${PROJECT_SOURCE_DIR}/src/digest)
include_directories(${PROJECT_SOURCE_DIR}/src/filescanner)

file(GLOB_RECURSE SRC_DIR_LIST "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_compile_options(-DUNICODE) 

if(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Release"))
elseif(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo"))
elseif(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
else()
SET(CMAKE_BUILD_TYPE "Debug")
endif()

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    SET(SYSTEM_TYPE "linux")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wall -O3")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -O3")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO  "${CMAKE_C_FLAGS_RELWITHDEBINFO} -Wall -O3 -g")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -Wall -O3 -g")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -O0 -g -ggdb")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -O0 -g -ggdb")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Windows")
    SET(SYSTEM_TYPE "windows")
    IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
        SET(CMAKE_SYSTEM_PROCESSOR x64)
    ELSE()
        SET(CMAKE_SYSTEM_PROCESSOR x86)
    ENDIF()
ENDIF ()

add_executable(Test ${SRC_DIR_LIST})

