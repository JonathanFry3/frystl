cmake_minimum_required(VERSION 3.8)

project(frystl)
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)            ## compile with C++17 support
endif()

if(NOT CMAKE_CXX_STANDARD_REQUIRED)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

include_directories(${PROJECT_SOURCE_DIR})

add_executable(test-sv tests/test-sv.cpp frystl.natvis)
add_executable(test-sd tests/test-sd.cpp frystl.natvis)
add_executable(test-mfv tests/test-mfv.cpp frystl.natvis)
