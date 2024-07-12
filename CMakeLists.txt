cmake_minimum_required(VERSION 3.23...3.29)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
if (CMAKE_CXX_STANDARD LESS 20)
    MESSAGE(FATAL_ERROR "C++20 or higher required!")
elseif (NOT EXISTS CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif ()

project(WaraPSClient VERSION 1.0
        DESCRIPTION "WaraPS Client"
        LANGUAGES CXX
)

find_package(PahoMqttCpp REQUIRED)
find_package(nlohmann_json REQUIRED)

add_library(WaraPSClient STATIC)
add_library(WaraPSClient::WaraPSClient ALIAS WaraPSClient)
target_sources(WaraPSClient
        PRIVATE
        src/wara_ps_client.cc

        PUBLIC
        FILE_SET api
        TYPE HEADERS
        BASE_DIRS src
        FILES src/wara_ps_client.h
)
target_link_libraries(WaraPSClient PUBLIC
        PahoMqttCpp::paho-mqttpp3
        nlohmann_json::nlohmann_json
)

include(GNUInstallDirs)

install(TARGETS WaraPSClient
        EXPORT WaraPSClientTargets
        FILE_SET api
)

install(EXPORT WaraPSClientTargets
        FILE WaraPSClientTargets.cmake
        NAMESPACE WaraPSClient::
        DESTINATION cmake/WaraPSClient
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
        cmake/WaraPSClientConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/WaraPSClientConfig.cmake
        INSTALL_DESTINATION cmake/WaraPSClient
)

write_basic_package_version_file(
        WaraPSClientConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/WaraPSClientConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/WaraPSClientConfigVersion.cmake
        DESTINATION cmake/WaraPSClient
)

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/WaraPSClient.pc.in
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/WaraPSClient.pc
        @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cmake/WaraPSClient.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
