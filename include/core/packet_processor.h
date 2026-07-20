//
// Created by xint2 on 19/07/2026.
//

#ifndef IVPN_PACKET_PROCESSOR_H
#define IVPN_PACKET_PROCESSOR_H
#include <cstdint>
#include <span>
#include <unordered_map>
#include  <functional>
#include "../../include/core/wintun.h"
#include "../../include/core/socks5.h"
namespace ivpn::core {
    struct tcpStreamKey {
        std::string src_ip;
        std::string dst_ip;
        uint16_t src_port;
        uint16_t dst_port;
        bool operator==(const tcpStreamKey &) const = default;
    };
    struct tcpStreamKey_hash {
        size_t operator()(const tcpStreamKey& k) const {
            auto h1 = std::hash<std::string>{}(k.src_ip);
            auto h2 = std::hash<std::string>{}(k.dst_ip);
            return h1 ^ (h2 << 1) ^ (k.src_port << 2) ^ (k.dst_port << 3);
        }
    };
    struct tcpStream {
        socks5Client socks;
        bool connected = false;
    };

    class packetProcessor {
    public:
        packetProcessor(WintunSession& session, socks5Client& socks);
        void start();
        void stop();


    private:
        WintunSession& session_;
        socks5Client& socks_;
        bool running_ = false;
        void process_loop();
        void handle_ipv4_packet(std::span<const uint8_t>packet);
        std::unordered_map<tcpStreamKey, tcpStream, tcpStreamKey_hash> streams_;
    };


}
#endif //IVPN_PACKET_PROCESSOR_H