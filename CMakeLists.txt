cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_COMPILER "gcc")

include_directories(${PROJECT_CURRENT_SOURCE_DIR}/include)

project(MQTT)

add_executable(mqtt src/main.c)