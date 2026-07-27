//
// Created by xint2 on 27/07/2026.
//
#include "../../include/core/dns_interceptor.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <spdlog/spdlog.h>

namespace ivpn::core {
    dnsInterceptor::dnsInterceptor(uint16_t tor_dns_port) {
        sock_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock_< 0) {
            spdlog::error("Failed to create DNS interceptor socket");
        }
        DWORD timeout = 2000;
        setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout,sizeof(timeout));
    }
    dnsInterceptor::~dnsInterceptor() {
        if (sock_>= 0) {
            closesocket(sock_);
        }
    }
    std::vector<uint8_t> dnsInterceptor::resolve(std::span<const uint8_t> query_payload) {
        if (sock_ < 0 || query_payload.empty()) return {};
        sockaddr_in tor_addr{};
        tor_addr.sin_family = AF_INET;
        tor_addr.sin_port = htons(tor_dns_port_);
        inet_pton(AF_INET, "127.0.0.1", &tor_addr.sin_addr);

        int sent = sendto(sock_,(const char*)query_payload.data(), (int)query_payload.size(), 0, (sockaddr*)&tor_addr, sizeof(tor_addr));

        if (sent < 0 ) {
            spdlog::error("Failed to send DNS querry to tor ");
            return {};
        }
        char buf[512];
        sockaddr_in from_addr{};
        int from_len = sizeof(from_addr);
        int received = recvfrom(sock_,buf, sizeof(buf), 0, (sockaddr*)&from_addr, &from_len);
        if (received > 0 ) {
            return std::vector<uint8_t>(buf, buf+ received);

        }
        spdlog::debug("DNS query to Tor time out");
        return {};
    }

}