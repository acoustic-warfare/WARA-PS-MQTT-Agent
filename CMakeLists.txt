cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_COMPILER "gcc")

project(MQTT)

add_executable(mqtt src/main.c)

target_include_directories(mqtt PRIVATE include)