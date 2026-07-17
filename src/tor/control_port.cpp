//
// Created by xint2 on 17/07/2026.
//

#include "../../include/tor/control_port.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <spdlog/spdlog.h>

namespace  ivpn::tor {
    controlPort::controlPort(const std::string &host, uint16_t port) : host_(host), port_(port){

    }
    controlPort::~controlPort() {
        disconnect();
    }
    bool controlPort::connect() {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,2), & wsa) != 0) {
            return false;
        }
        sock_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_ < 0) {
            return false;
        }
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_);
        inet_pton(AF_INET, host_.c_str(), &addr.sin_addr);

        return ::connect(sock_, (sockaddr*)&addr, sizeof(addr)) >=0;
    }
    bool controlPort::authenticate() {
        auto resp = send("AUTHENTICATE \"\"");
        return resp && resp->find("250") == 0;
    }
    void controlPort::disconnect() {
        if (sock_ >= 0) {
            closesocket(sock_);
            sock_ = -1;
        }
        WSACleanup();
    }
    std::optional<std::string> controlPort::send(const std::string &cmd) {
        if (sock_ < 0) return {};
        std::string msg = cmd + "\r\n";
        int sent = ::send(sock_, msg.data(),(int)msg.size(), 0);
        if (sent <= 0) return {};
        char buf[4096];
        int n = recv(sock_, buf , sizeof(buf) -1,0);
        if (n <= 0 ) return {};
        buf[n] = '\0';
        return std::string(buf);

    }






}