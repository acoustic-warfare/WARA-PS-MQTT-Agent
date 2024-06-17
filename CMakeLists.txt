cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 17)
project(mqttSender VERSION 1.0.0)

find_package(PahoMqttCpp REQUIRED)
find_package(eclipse-paho-mqtt-c REQUIRED)

add_library(mqtt_client src/mqtt_client.cc)
target_link_libraries(mqtt_client paho-mqttpp3 paho-mqtt3as)