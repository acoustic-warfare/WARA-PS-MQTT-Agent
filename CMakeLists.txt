cmake_minimum_required(VERSION 3.23...3.29)

if (${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif ()

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(WaraPSClient VERSION 1.0
        DESCRIPTION "WaraPS Client"
        LANGUAGES CXX
)

find_package(PahoMqttCpp REQUIRED)
find_package(nlohmann_json REQUIRED)


set(CLANG_WARNINGS
        -Wall
        -Wextra # reasonable and standard
        -Wextra-semi # Warn about semicolon after in-class function definition.
        -Wshadow # warn the user if a variable declaration shadows one from a parent context
        -Wnon-virtual-dtor # warn the user if a class with virtual functions has a non-virtual destructor. This helps
        # catch hard to track down memory errors
        -Wold-style-cast # warn for c-style casts
        -Wcast-align # warn for potential performance problem casts
        -Wunused # warn on anything being unused
        -Woverloaded-virtual # warn if you overload (not override) a virtual function
        -Wpedantic # warn if non-standard C++ is used
        -Wconversion # warn on type conversions that may lose data
        -Wsign-conversion # warn on sign conversions
        -Wnull-dereference # warn if a null dereference is detected
        -Wdouble-promotion # warn if float is implicit promoted to double
        -Wformat=2 # warn on security issues around functions that format output (ie printf)
        -Wimplicit-fallthrough # warn on statements that fallthrough without an explicit annotation
)

set(GCC_WARNINGS
        ${CLANG_WARNINGS}
        -Wmisleading-indentation # warn if indentation implies blocks where blocks do not exist
        -Wduplicated-cond # warn if if / else chain has duplicated conditions
        -Wduplicated-branches # warn if if / else branches have duplicated code
        -Wlogical-op # warn about logical operations being used where bitwise were probably wanted
        -Wuseless-cast # warn if you perform a cast to the same type
)

if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    set(PROJECT_WARNINGS_CXX ${CLANG_WARNINGS})
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(PROJECT_WARNINGS_CXX ${GCC_WARNINGS})
else ()
    message(AUTHOR_WARNING
            "No compiler warnings set for CXX compiler: '${CMAKE_CXX_COMPILER_ID}'"
    )
endif ()

add_library(WaraPSClient SHARED)
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

target_compile_options(WaraPSClient PRIVATE
        ${PROJECT_WARNINGS_CXX}
)

include(GNUInstallDirs)

install(TARGETS WaraPSClient
        EXPORT WaraPSClientTargets
        FILE_SET api
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

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/WaraPSClient.pc.in
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/WaraPSClient.pc
        @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cmake/WaraPSClient.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
