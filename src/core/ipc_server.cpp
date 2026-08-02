//
// Created by xint2 on 29/07/2026.
//
#include "../../include/core/ipc_server.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace ivpn::core {
    ipcServer::ipcServer(const std::string& pipe_name) : pipe_name_(pipe_name) {}
    ipcServer::~ipcServer() {
        stop();
    }
    void ipcServer::start() {
        if (running_) return;
        running_ = true;
        server_thread_= std::thread(&ipcServer::listen_loop, this);
    }
    void ipcServer::stop() {
        running_ = false;
        if (pipe_ != INVALID_HANDLE_VALUE) {
            HANDLE hClient = CreateFileA(pipe_name_.c_str(), GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0,NULL);
            if (hClient != INVALID_HANDLE_VALUE) {
                CloseHandle(hClient);
            }
        }
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
    }
    void ipcServer::listen_loop(){
        SECURITY_DESCRIPTOR sd;
        InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle = FALSE;

        while (running_) {
            pipe_ = CreateNamedPipeA(
                pipe_name_.c_str(),
                PIPE_ACCESS_DUPLEX,
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE| PIPE_WAIT,
                PIPE_UNLIMITED_INSTANCES,
                4096,4096,0,&sa);

            if (pipe_ == INVALID_HANDLE_VALUE) {
                spdlog::error("Failed to create IPC pipe. Error: {}", GetLastError());
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;;
            }
            if (ConnectNamedPipe(pipe_,NULL) != FALSE || GetLastError() == ERROR_PIPE_CONNECTED) {
                char buffer[4096];
                DWORD bytes_read;

                if (ReadFile(pipe_, buffer,sizeof(buffer)-1,&bytes_read,NULL)) {
                    buffer[bytes_read] = '\0';
                    try {
                        auto j = nlohmann::json::parse(buffer);
                        std::string cmd = j["command"];
                        if (cmd == "connect" && on_connect_) {
                            on_connect_();
                        }
                        else if (cmd == "disconnect" && on_disconnect_) {
                            on_disconnect_();
                        }else if (cmd == "change_city" && on_change_city_) on_change_city_(j["city"]);
                        std::string ack = R"({"status":"ok"})";
                        DWORD bytes_written;
                        WriteFile(pipe_, ack.c_str(),(DWORD)ack.size(), &bytes_written, NULL);

                    }
                    catch (const std::exception& e) {
                        spdlog::error("IPC JSON parse error: {}", e.what());
                    }
                }
            }
            FlushFileBuffers(pipe_);
            DisconnectNamedPipe(pipe_);
            CloseHandle(pipe_);
            pipe_ = INVALID_HANDLE_VALUE;

        }
    }
}
