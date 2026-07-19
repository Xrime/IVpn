//
// Created by xint2 on 18/07/2026.
//

#ifndef IVPN_SOCKS5_H
#define IVPN_SOCKS5_H
#include <cstdint>
#include <string>
#include <optional>
#include <span>
#include <vector>

class socks5Client {
public:
    socks5Client(const std::string& host = "127.0.0.1", uint16_t port =9050);
    bool connect(const std::string& target_host, uint16_t target_port);
    bool send_packet(std::span<const uint8_t> data);
    std::optional<std::pmr::vector<uint8_t>> receive_packet(uint32_t timeout = 1000);
    void close();
private:
    std::string host_;
    uint16_t port_;
    int sock_ = -1;
    bool handshake();
    bool sendConnectRequest(const std::string& host, uint16_t port);
};
#endif //IVPN_SOCKS5_H