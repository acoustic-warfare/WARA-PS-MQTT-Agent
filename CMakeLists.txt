cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 11)
project(mqttSender VERSION 1.0.0)

find_package(PahoMqttCpp REQUIRED)
find_package(eclipse-paho-mqtt-c REQUIRED)

add_executable(main src/main.cc)
target_link_libraries(main paho-mqttpp3 paho-mqtt3as)