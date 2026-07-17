cmake_minimum_required(VERSION 3.22)
project(IVpn VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg toolchain
if(NOT CMAKE_TOOLCHAIN_FILE AND EXISTS "${CMAKE_SOURCE_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()

find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

add_executable(ivpn src/core/config.cpp
        main.cpp
        src/core/wintun.cpp
        include/core/wintun.h
        src/core/wintun.cpp
        src/core/wintun.cpp)
target_link_libraries(ivpn PRIVATE nlohmann_json::nlohmann_json spdlog::spdlog)
target_include_directories(ivpn PRIVATE src)