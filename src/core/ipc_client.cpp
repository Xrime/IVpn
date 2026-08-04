//
// Created by xint2 on 29/07/2026.
//

#include "../../include/core/ipc_client.h"
#include <windows.h>
#include <spdlog/spdlog.h>
#include <nlohmann//json.hpp>

namespace ivpn::core {
    ipcClient::ipcClient(const std::string &pipe_name):pipe_name_(pipe_name) {

    }
    bool ipcClient::send_command(const std::string &command, const std::string &city) {
        nlohmann::json j;
        j["command"] = command;
        if (!city.empty()) {
            j["city"] = city;
        }
        std::string payload = j.dump();

        HANDLE hPipe = CreateFileA(
            pipe_name_.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0, NULL, OPEN_EXISTING, 0,NULL
        );
        if (hPipe == INVALID_HANDLE_VALUE) {
            spdlog::error("Failed to connect to IVpn Daemon. Error code: {}", GetLastError());
            return false;
        }
        DWORD byte_written;
        if (!WriteFile(hPipe, payload.c_str(), (DWORD)payload.size(), &byte_written, NULL)) {
            return false;
        }
        char buffer[1024];
        DWORD bytes_read;
        if (ReadFile(hPipe, buffer, sizeof(buffer)- 1, &bytes_read, NULL)) {
            buffer[bytes_read] = '\0';
            spdlog::debug("Deamon replied: {}", buffer);
        }
        CloseHandle(hPipe);
        return true;
    }
    std::vector<std::string> ipcClient::get_cities() {
        nlohmann::json j;
        j["command"] = "get_cities";
        std::string payload = j.dump();

        HANDLE hPipe = CreateFileA(pipe_name_.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hPipe == INVALID_HANDLE_VALUE) {
            spdlog::error("failed to connect to IVpn Deamon. error code : {} ",GetLastError());
            return {};
        }
        DWORD byte_written;
        if (!WriteFile(hPipe, payload.c_str(), (DWORD)payload.size(), &byte_written, NULL)) {
            CloseHandle(hPipe);
            return {};
        }
        char buffer[4096];
        DWORD bytes_read;
        std::vector<std::string> cities;
        if (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytes_read, NULL)) {
            buffer[bytes_read] = '\0';
            try {
                auto response = nlohmann::json::parse(buffer);
                if (response.contains("cities")) {
                    cities = response["cities"].get<std::vector<std::string>>();
                }
            }catch (const std::exception& e) {
                    spdlog::error("Failed to parse city list: {}",e.what());
                }
            }
        CloseHandle(hPipe);
        return cities;
        }
    }