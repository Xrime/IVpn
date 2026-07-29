//
// Created by xint2 on 29/07/2026.
//

#ifndef IVPN_IPC_SERVER_H
#define IVPN_IPC_SERVER_H

#include <string>
#include <functional>
#include <thread>
#include <windows.h>
#include <atomic>

namespace ivpn::core {
    class ipcServer {
    public:
        ipcServer(const std::string& pipe_name = "\\\\.\\pipe\\ivpn_control");
        ~ipcServer();
        void start();
        void stop();
        void setOnConnect(std::function<void()> cb ){on_connect_ = cb;}
        void seOnDisconnect(std::function<void()> cb) {on_disconnect_ = cb;}
        void setOnChangeCity(std::function<void(const std::string&)> cb){on_change_city_= cb;}

    private:
        std::string pipe_name_;
        std::atomic<bool> running_{false};
        std::thread server_thread_;
        std::function<void()> on_connect_;
        HANDLE pipe_ = INVALID_HANDLE_VALUE;
        std::function<void()> on_disconnect_;
        std::function<void(const std::string&)> on_change_city_;
        void listen_loop();
    };
}
#endif //IVPN_IPC_SERVER_H