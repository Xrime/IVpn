cmake_minimum_required(VERSION 3.22)
project(IVpn VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg toolchain
if(NOT CMAKE_TOOLCHAIN_FILE AND EXISTS "${CMAKE_SOURCE_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()

find_package(nlohmann_json REQUIRED)
find_package(maxminddb REQUIRED)
find_package(spdlog REQUIRED)

add_executable(ivpn src/core/config.cpp
        main.cpp
        src/core/wintun.cpp
        include/core/wintun.h
        src/core/wintun.cpp
        src/core/wintun.cpp
        include/tor/control_port.h
        src/tor/control_port.cpp
        include/core/geoip.h
        src/core/geoip.cpp
        include/core/exit_selector.h
        src/core/exit_selector.cpp
        include/core/circuit_builder.h
        src/core/circuit_builder.cpp
        include/core/tor_launcher.h
        src/core/tor_launcher.cpp
        include/core/socks5.h
        src/core/socks5.cpp
        include/core/packet_processor.h
        src/core/packet_processor.cpp
        include/core/bootstrap_waiter.h
        src/core/bootstrap_waiter.cpp
        include/core/route_manager.h
        src/core/route_manager.cpp)
target_link_libraries(ivpn PRIVATE ws2_32 nlohmann_json::nlohmann_json spdlog::spdlog  maxminddb::maxminddb)
target_include_directories(ivpn PRIVATE src)