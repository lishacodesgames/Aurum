cmake_minimum_required(VERSION 3.10)
project(Orum LANGUAGES CXX)

file(GLOB_RECURSE SRC "${CMAKE_SOURCE_DIR}/src/*.cpp")
add_executable(${PROJECT_NAME} ${SRC})
