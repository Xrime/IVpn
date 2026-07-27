//
// Created by xint2 on 19/07/2026.
//
#include "../../include/core/packet_processor.h"
#include <spdlog/spdlog.h>
#include <thread>
#include <cstring>
#include "../../include/core/dns_interceptor.h"

namespace ivpn::core {

    packetProcessor::packetProcessor(WintunSession& session, const std::string& socks_host, uint16_t socks_port)
        : session_(session), socks_host_(socks_host), socks_port_(socks_port) {
    }

    void packetProcessor::start() {
        running_ = true;
        std::thread(&packetProcessor::process_loop, this).detach();
        recvThread_ = std::thread(&packetProcessor::response_loop, this);
    }

    void packetProcessor::stop() {
        running_ = false;
        if (recvThread_.joinable()) {
            recvThread_.join();
        }
    }

    uint16_t packetProcessor::checksum(uint16_t* data, size_t len) {
        uint32_t sum = 0;
        for (size_t i = 0; i < len; i++) {
            sum += data[i];
        }
        while (sum >> 16) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
        return static_cast<uint16_t>(~sum);
    }

    std::vector<uint8_t> packetProcessor::buildTCPPacket(const tcpStreamKey& key,
                                                         uint8_t tcp_flags,
                                                         std::span<const uint8_t> payload) {
        uint8_t src_ip[4], dst_ip[4];
        sscanf(key.remote_ip.c_str(), "%hhu.%hhu.%hhu.%hhu", &src_ip[0], &src_ip[1], &src_ip[2], &src_ip[3]);
        sscanf(key.local_ip.c_str(), "%hhu.%hhu.%hhu.%hhu", &dst_ip[0], &dst_ip[1], &dst_ip[2], &dst_ip[3]);

        std::vector<uint8_t> packet;
        packet.reserve(20 + 20 + payload.size());

        // IP header
        packet.push_back(0x45); packet.push_back(0x00);
        packet.push_back(0); packet.push_back(0);
        packet.push_back(0); packet.push_back(0x40);
        packet.push_back(64); packet.push_back(6);
        packet.push_back(0); packet.push_back(0);
        for (int i = 0; i < 4; i++) packet.push_back(src_ip[i]);
        for (int i = 0; i < 4; i++) packet.push_back(dst_ip[i]);

        // TCP header
        packet.push_back(key.remote_port >> 8);
        packet.push_back(key.remote_port & 0xFF);
        packet.push_back(key.local_port >> 8);
        packet.push_back(key.local_port & 0xFF);
        packet.push_back(0); packet.push_back(0); packet.push_back(0); packet.push_back(0);
        packet.push_back(0); packet.push_back(0); packet.push_back(0); packet.push_back(0);
        packet.push_back(0x50); packet.push_back(tcp_flags);
        packet.push_back(0); packet.push_back(2);
        packet.push_back(0); packet.push_back(0);

        packet.insert(packet.end(), payload.begin(), payload.end());
        return packet;
    }
    std::vector<uint8_t> packetProcessor::buildUDPPacket(const std::string &src_ip, uint16_t src_port, const std::string &dst_ip, uint16_t dst_port, std::span<const uint8_t> payload) {
        uint8_t s_ip[4], d_ip[4];
        scanf(src_ip.c_str(), "%hhu.%hhu.%hhu.%hhu", &s_ip[0], &s_ip[1],&s_ip[2],s_ip[3]);
        scanf(dst_ip.c_str(), "%hhu.%hhu.%hhu.%hhu", &d_ip[0], &d_ip[1], &d_ip[2], &d_ip[3]);

        std::vector<uint8_t> packet;
        uint16_t total_len = 20 + 8 + (uint16_t) payload.size();
        packet.reserve(total_len);

        packet.push_back(0x45);
        packet.push_back(0x00);
        packet.push_back(total_len >> 8);
        packet.push_back(total_len & 0xFF);
        packet.push_back(0);
        packet.push_back(0);
        packet.push_back(0x40);
        packet.push_back(0);
        packet.push_back(64);
        packet.push_back(17);
        packet.push_back(0);
        packet.push_back(0);

        for (int i = 0 ;i < 4; i++ )packet.push_back(s_ip[i]);
        for (int i = 0; i < 4; i++) packet.push_back(d_ip[i]);
        uint16_t udpLen = 8 + (uint16_t)payload.size();

        //udp header (8 bytes)
        packet.push_back(src_port >> 8);
        packet.push_back(src_port & 0xFF);
        packet.push_back(dst_port >> 8);
        packet.push_back(dst_port & 0xFF);
        packet.push_back(udpLen >> 8);
        packet.push_back(udpLen & 0xFF);

        packet.push_back(0);
        packet.push_back(0);
        packet.insert(packet.end(), payload.begin(), payload.end());
        return packet;
    }


    void packetProcessor::process_loop() {
        while (running_) {
            size_t size = 0;
            auto packet = session_.receive_packet(1000, &size);
            if (size > 0 && !packet.empty()) {
                handle_ipv4_packet(packet);
            }
        }
    }

    void packetProcessor::handle_ipv4_packet(std::span<const uint8_t> packet) {
        if (packet.size() < 20) return;
        uint8_t version = packet[0] >> 4;
        if (version != 4) return;

        uint8_t protocol = packet[9];
        uint16_t total_len = (packet[2] << 8) | packet[3];
        size_t ip_header_len = (packet[0] & 0x0F) * 4;
        std::string src_ip = fmt::format("{}.{}.{}.{}", packet[12], packet[13], packet[14], packet[15]);
        std::string dst_ip = fmt::format("{}.{}.{}.{}", packet[16], packet[17], packet[18], packet[19]);
        if (protocol == 6) {
            uint16_t src_port = (packet[20] << 8) | packet[21];
            uint16_t dst_port = (packet[22] << 8) | packet[23];

            tcpStreamKey key{src_ip, dst_ip, src_port, dst_port};

            std::lock_guard<std::mutex> lock(streams_mutex_);
            auto& stream = streams_[key];
            if (!stream) stream = std::make_unique<tcpStream>();

            uint8_t tcp_flags = packet[ip_header_len + 13];
            bool syn = tcp_flags & 0x02;
            bool ack = tcp_flags & 0x10;
            size_t tcp_header_len = ((packet[ip_header_len + 12] >> 4) & 0x0F) * 4;
            size_t payload_len = total_len - ip_header_len - tcp_header_len;

            if (syn && !ack) {
                spdlog::info("TCP SYN {}:{} -> {}:{}", src_ip, src_port, dst_ip, dst_port);
                stream->socks = std::make_unique<socks5Client>(socks_host_, socks_port_);
                stream->connected = stream->socks->connect(dst_ip, dst_port);
                if (!stream->connected) {
                    spdlog::error("Failed to connect {}:{}", dst_ip, dst_port);
                }
            }

            if (payload_len > 0 && stream->connected) {
                auto payload = packet.subspan(ip_header_len + tcp_header_len, payload_len);
                stream->socks->send_packet(payload);
            }
        }
        else if (protocol == 17 ) {
            if (packet.size() < ip_header_len + 8) return;

            uint16_t src_port = (packet[ip_header_len] << 8) | packet[ip_header_len  + 1];
            uint16_t dst_port = (packet[ip_header_len + 2] << 8)| packet[ip_header_len + 3];
            uint16_t udpLen = (packet[ip_header_len + 4] << 8) | packet[ip_header_len + 5];
            if (dst_port == 53) {
                size_t payloadLen = udpLen - 8;
                if (ip_header_len + 8 + payloadLen <= packet.size()) {
                    std::vector<uint8_t> dnsPayload(packet.begin() + ip_header_len + 8, packet.begin() + ip_header_len + 8 + payloadLen);
                    spdlog::info("Intercepted DNS query from {} to {}", src_ip, dst_port);

                    std::thread([this, src_ip, src_port, dst_ip, dst_port, dnsPayload]() {
                        dnsInterceptor dns(9053);
                        auto response = dns.resolve(dnsPayload);
                        if (!response.empty()) {
                            auto reply = buildUDPPacket(dst_ip,dst_port, src_ip, src_port, response);
                            session_.send_packet(reply);
                        }
                    }).detach();
                }
                else {
                    spdlog::debug("Dropped no-DNS UDP packet to {}:{}", dst_ip,dst_port);

                }
            }
        }
    }

    void packetProcessor::response_loop() {
        while (running_) {
            std::vector<std::pair<tcpStreamKey, std::vector<uint8_t>>> responses;

            {
                std::lock_guard<std::mutex> lock(streams_mutex_);
                for (auto& [key, stream] : streams_) {
                    if (stream && stream->connected) {
                        auto resp = stream->socks->receive_packet(10);
                        if (resp && resp->size() > 0) {
                            responses.push_back({key, std::vector<uint8_t>(resp->begin(), resp->end())});
                        }
                    }
                }
            }

            for (auto& [key, payload] : responses) {
                session_.send_packet(buildTCPPacket(key, 0x10, payload));
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

} // namespace ivpn::core