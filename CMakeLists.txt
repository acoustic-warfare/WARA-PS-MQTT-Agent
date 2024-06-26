cmake_minimum_required(VERSION 3.9)
set(CMAKE_CXX_STANDARD 20)
project(WaraPSClient VERSION 1.0.0)

# Tacka chatGPT vetja

find_package(PahoMqttCpp REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(eclipse-paho-mqtt-c REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(Boost REQUIRED)

configure_file(cmake/waraPsClient.pc.in waraPsClient.pc @ONLY)

add_library(WaraPSClient src/wara_ps_client.cc)
add_executable(WaraPSClientTest src/wara_ps_client_test.cc src/wara_ps_client.cc)

target_link_libraries(WaraPSClientTest
        WaraPSClient
)

set_target_properties(WaraPSClient PROPERTIES
        PUBLIC_HEADER src/wara_ps_client.h
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        EXPORT_NAME WaraPSClient
)
target_include_directories(WaraPSClient PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(WaraPSClient
        PahoMqttCpp::paho-mqttpp3
        eclipse-paho-mqtt-c::paho-mqtt3as
        nlohmann_json::nlohmann_json
)

include(GNUInstallDirs)
install(TARGETS WaraPSClient
        EXPORT WaraPSClientTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${CMAKE_BINARY_DIR}/waraPsClient.pc
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig
)

# Export the targets to a file
install(EXPORT WaraPSClientTargets
        FILE WaraPSClientTargets.cmake
        NAMESPACE WaraPSClient::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/WaraPSClient
)

# Create the WaraPSClientConfig.cmake and WaraPSClientConfigVersion.cmake files
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/WaraPSClientConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/WaraPSClientConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/WaraPSClientConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/WaraPSClient
)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/WaraPSClientConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/WaraPSClientConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/WaraPSClient
)
