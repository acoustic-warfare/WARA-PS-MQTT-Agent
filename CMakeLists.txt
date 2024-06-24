cmake_minimum_required(VERSION 3.9)
set(CMAKE_CXX_STANDARD 20)
project(waraPsClient VERSION 1.0.0)

find_package(PahoMqttCpp REQUIRED)
find_package(eclipse-paho-mqtt-c REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(Boost REQUIRED)

configure_file(waraPsClient.pc.in waraPsClient.pc @ONLY)

add_library(WaraPSClient src/waraps_client.cc)
set_target_properties(WaraPSClient PROPERTIES PUBLIC_HEADER include/waraps_client.h VERSION ${PROJECT_VERSION})
target_include_directories(WaraPSClient PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(WaraPSClient paho-mqttpp3 paho-mqtt3as nlohmann_json::nlohmann_json)

include(GNUInstallDirs)
install(TARGETS WaraPSClient
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_BINARY_DIR}/waraPsClient.pc
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
