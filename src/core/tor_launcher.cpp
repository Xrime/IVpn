//
// Created by xint2 on 18/07/2026.
//
#include "../../include/core/tor_launcher.h"
#include <windows.h>
#include <spdlog/spdlog.h>

#include "../../include/tor/control_port.h"

torLauncher::torLauncher(const std::string &tor_path, const std::string &data_dir) : tor_path_(tor_path), data_dir_(data_dir){

}
bool torLauncher::start(uint16_t socks_port, uint16_t control_port, uint16_t dns_port) {
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    std::string cmd = fmt::format("\"{}\" --SOCKSPort {} --ControlPort {} --DNSPort {} --CookieAuthentication 1 --DataDirectory \"{}\" --Log \"notice file tor.log\"",tor_path_, socks_port, control_port, dns_port, data_dir_);

    if (!CreateProcessA(tor_path_.c_str(), &cmd[0], nullptr, nullptr, FALSE, CREATE_NEW_PROCESS_GROUP, nullptr, nullptr, &si, &pi)) {
        spdlog::error("Failed to start tor: {}", GetLastError());
        return false;
    }
    process_handle_ = new  PROCESS_INFORMATION(pi);
    spdlog::info("Tor starter (PID {})", pi.dwProcessId);

    return false;

}
bool torLauncher::stop() {
    if (!process_handle_) {
        return true;
    }
    auto pi = static_cast<PROCESS_INFORMATION*>(process_handle_);
    if (TerminateProcess(pi->hProcess, 0)) {
        WaitForSingleObject(pi->hProcess, 5000);
        CloseHandle(pi->hProcess);
        CloseHandle(pi->hThread);
        delete pi;
        process_handle_ = nullptr;
        spdlog::info("Tor stopped");
        return true;
    }
    return false;
}
bool torLauncher::is_running() const {
    if (!process_handle_) return false;
    auto pi= static_cast<PROCESS_INFORMATION*>(process_handle_);
    return WaitForSingleObject(pi->hProcess, 0)==WAIT_TIMEOUT;
}

