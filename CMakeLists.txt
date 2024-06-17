cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 17)
project(mqttSender VERSION 1.0.0)

find_package(PahoMqttCpp REQUIRED)
find_package(eclipse-paho-mqtt-c REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

add_executable(mqtt_client src/mqtt_client.cc src/main.cc)
target_link_libraries(mqtt_client paho-mqttpp3 paho-mqtt3as nlohmann_json::nlohmann_json)