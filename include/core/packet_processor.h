//
// Created by xint2 on 19/07/2026.
//

#ifndef IVPN_PACKET_PROCESSOR_H
#define IVPN_PACKET_PROCESSOR_H
#include <cstdint>
#include <span>
#include <unordered_map>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include "../../include/core/wintun.h"
#include "../../include/core/socks5.h"

namespace ivpn::core {
    struct tcpStreamKey {
        std::string local_ip;
        std::string remote_ip;
        uint16_t local_port;
        uint16_t remote_port;
        bool operator==(const tcpStreamKey &) const = default;
    };
    struct tcpStreamKey_hash {
        size_t operator()(const tcpStreamKey& k) const {
            auto h1 = std::hash<std::string>{}(k.local_ip);
            auto h2 = std::hash<std::string>{}(k.remote_ip);
            return h1 ^ (h2 << 1) ^ (k.local_port << 2) ^ (k.remote_port << 3);
        }
    };
    enum class tcpState {
        Listen,
        SynReceived,
        Established,
        Closed
    };

    struct tcpStream {
        tcpStream() = default;
        tcpStream(const tcpStream&)= delete;
        tcpStream& operator = (const tcpStream&) = delete;
        std::unique_ptr<socks5Client> socks;
        bool connected = false;
        bool connecting = false;
        tcpState state = tcpState::Listen;
        uint32_t client_seq = 0;
        uint32_t vpn_seq = 0;
        std::chrono::steady_clock::time_point last_activity{std::chrono::steady_clock::now()};

    };
    class packetProcessor {
    public:
        packetProcessor(WintunSession& session, const std::string& socks_host, uint16_t socks_port);
        void start();
        void stop();

    private:
        WintunSession& session_;
        std::string socks_host_;
        uint16_t socks_port_;
        bool running_ = false;
        std::mutex streams_mutex_;
        void process_loop();
        void handle_ipv4_packet(std::span<const uint8_t>packet);
        void response_loop();
        std::unordered_map<tcpStreamKey, std::unique_ptr<tcpStream>, tcpStreamKey_hash> streams_;
        std::thread recvThread_;
        std::vector<uint8_t> buildTCPPacket(const tcpStreamKey& key,
                                          uint8_t tcp_flags,
                                          uint32_t seq,
                                          uint32_t ack,
                                          std::span<const uint8_t> payload);
        uint16_t checksum(uint16_t* data, size_t len);
        std::vector<uint8_t> buildUDPPacket(const std::string& src_ip,uint16_t src_port, const std::string& dst_ip, uint16_t dst_port, std::span<const uint8_t> payload);
        void cleanupStateStreams();

    };

}
#endif //IVPN_PACKET_PROCESSOR_H