//
// Created by xint2 on 18/07/2026.
//
#include "../../include/core/socks5.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <spdlog/spdlog.h>

socks5Client::socks5Client(const std::string &host, uint16_t port) : host_(host),port_(port){

}
bool socks5Client::connect(const std::string &target_host, uint16_t target_port) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        return false;
    }
    sock_= socket(AF_INET, SOCK_STREAM,0);
    if (sock_< 0) {
        return false;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    inet_pton(AF_INET, host_.c_str(), &addr.sin_addr);
    if (::connect(sock_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        spdlog::error("SOCKS5 connect failed");
        close();
        return false;
    }
    if (!handshake()) {
        close();
        return false;
    }
    return sendConnectRequest(target_host, target_port);
}
bool socks5Client::handshake() {
    uint8_t greeting[] = {0x05,0x01,0x00};
    if (send(sock_, (char*)greeting, sizeof(greeting), 0)<0) {
        return false;
    }
    uint8_t response[2];
    if (recv(sock_, (char*)response, 2,0) < 2) return false;

    return response[1] == 0x00;
}
bool socks5Client::sendConnectRequest(const std::string &host, uint16_t port) {
    std::vector<uint8_t> req;
    req.push_back(0x05);
    req.push_back(0x01);
    req.push_back(0x00);
    req.push_back(0x03);
    req.push_back(host.size());
    req.insert(req.end(), host.begin(), host.end());
    req.push_back(port>>8);
    req.push_back(port & 0xFF);

    if (send(sock_, (char*)req.data(), req.size(), 0) < 0) {
        return  false;
    }
    uint8_t resp[256];
    int n = recv(sock_, (char*)resp, sizeof(resp), 0);
    if (n<6) return false;

    return resp[1] == 0x00;
}
bool socks5Client::send_packet(std::span<const uint8_t> data) {
    return ::send(sock_, (const char*)data.data(), (int)data.size(), 0) >= 0;
}
std::optional<std::pmr::vector<uint8_t> > socks5Client::receive_packet(uint32_t timeout) {
    char buf[4096];
    int n = recv(sock_, buf, sizeof(buf),0);
    if (n <=0) return {};
    return std::vector<uint8_t>(buf, buf + n);

}
void socks5Client::close() {
    if (sock_>= 0) {
        closesocket(sock_);
        sock_ = -1;
        WSACleanup();
    }
}





