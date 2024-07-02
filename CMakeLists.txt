cmake_minimum_required(VERSION 3.7...3.29)

if (${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif ()

project(WaraPSClient VERSION 1.0
        DESCRIPTION "WaraPS Client"
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(WaraPSClient src/wara_ps_client.cc)

find_package(PahoMqttCpp REQUIRED)
find_package(nlohmann_json REQUIRED)

target_link_libraries(WaraPSClient
        PUBLIC PahoMqttCpp::paho-mqttpp3
        PUBLIC nlohmann_json::nlohmann_json
)

include(GNUInstallDirs)

install(TARGETS WaraPSClient
        EXPORT WaraPSClientTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT WaraPSClientTargets
        FILE WaraPSClientTargets.cmake
        NAMESPACE WaraPSClient::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/WaraPSClient
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
        cmake/WaraPSClientConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/WaraPSClientConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/WaraPSClient
)

write_basic_package_version_file(
        WaraPSClientConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/WaraPSClientConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/WaraPSClientConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/WaraPSClient
)


