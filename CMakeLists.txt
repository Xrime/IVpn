cmake_minimum_required(VERSION 3.22)
project(IVpn VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


if(NOT CMAKE_TOOLCHAIN_FILE AND EXISTS "${CMAKE_SOURCE_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()

find_package(nlohmann_json REQUIRED)
find_package(maxminddb REQUIRED)
find_package(spdlog REQUIRED)

add_executable(ivpn_daemon
        src/core/service.cpp
        src/core/ipc_server.cpp
        src/core/config.cpp
        src/core/wintun.cpp
        src/tor/control_port.cpp
        src/core/geoip.cpp
        src/core/exit_selector.cpp
        src/core/circuit_builder.cpp
        src/core/tor_launcher.cpp
        src/core/socks5.cpp
        src/core/packet_processor.cpp
        src/core/bootstrap_waiter.cpp
        src/core/route_manager.cpp
        src/core/killswitch.cpp
        src/core/dns_interceptor.cpp
)

target_link_libraries(ivpn_daemon PRIVATE
        ws2_32
        iphlpapi
        fwpuclnt.lib
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        maxminddb::maxminddb
)
target_include_directories(ivpn_daemon PRIVATE src include)
target_compile_definitions(ivpn_daemon PRIVATE _WIN32_WINNT=0x0601)

add_executable(ivpn_client
        main.cpp
        src/UI/tui.cpp
        src/core/ipc_client.cpp
)


target_link_libraries(ivpn_client PRIVATE
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)
target_include_directories(ivpn_client PRIVATE src include)