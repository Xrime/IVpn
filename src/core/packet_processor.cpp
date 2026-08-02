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
                                                         uint32_t seq,
                                                         uint32_t ack,
                                                         std::span<const uint8_t> payload) {
        unsigned int s[4], d[4];
        sscanf(key.remote_ip.c_str(), "%u.%u.%u.%u", &s[0], &s[1], &s[2], &s[3]);
        sscanf(key.local_ip.c_str(), "%u.%u.%u.%u", &d[0], &d[1], &d[2], &d[3]);
        uint8_t src_ip[4] = { (uint8_t)s[0], (uint8_t)s[1], (uint8_t)s[2], (uint8_t)s[3] };
        uint8_t dst_ip[4] = { (uint8_t)d[0], (uint8_t)d[1], (uint8_t)d[2], (uint8_t)d[3] };

        bool is_syn = (tcp_flags & 0x02) != 0;
        uint16_t tcp_header_len = is_syn ? 24 : 20;
        uint16_t total_len = 20 + tcp_header_len + (uint16_t)payload.size();

        std::vector<uint8_t> packet;
        packet.reserve(total_len);

        // IP header
        packet.push_back(0x45); packet.push_back(0x00);
        packet.push_back(total_len >> 8);
        packet.push_back(total_len & 0xFF);
        packet.push_back(0); packet.push_back(0);
        packet.push_back(0x40); packet.push_back(0);
        packet.push_back(64); packet.push_back(6);
        packet.push_back(0); packet.push_back(0);
        for (int i = 0; i < 4; i++) packet.push_back(src_ip[i]);
        for (int i = 0; i < 4; i++) packet.push_back(dst_ip[i]);

        uint16_t ip_csum = checksum((uint16_t*)packet.data(), 10);
        packet[10] = ip_csum & 0xFF;
        packet[11] = ip_csum >> 8;
        
        // TCP header
        packet.push_back(key.remote_port >> 8);
        packet.push_back(key.remote_port & 0xFF);
        packet.push_back(key.local_port >> 8);
        packet.push_back(key.local_port & 0xFF);
        packet.push_back(seq >> 24); packet.push_back((seq >> 16) & 0xFF); packet.push_back((seq >> 8) &0xFF); packet.push_back(seq & 0xFF);
        packet.push_back(ack >> 24); packet.push_back((ack >> 16) & 0xFF); packet.push_back((ack >> 8) & 0xFF); packet.push_back(ack & 0xFF);
        uint8_t data_offset = (tcp_header_len / 4) << 4;
        packet.push_back(data_offset); packet.push_back(tcp_flags);
        packet.push_back(0xFA); packet.push_back(0xF0);
        packet.push_back(0); packet.push_back(0);
        packet.push_back(0); packet.push_back(0);
        if (is_syn) {
            packet.push_back(0x02);
            packet.push_back(0x04);
            packet.push_back(0x05); packet.push_back(0xB4);
        }

        packet.insert(packet.end(), payload.begin(), payload.end());
        std::vector<uint8_t> pseudo;
        for (int i = 0; i < 4; i++) {
            pseudo.push_back(src_ip[i]);
        }
        for (int i = 0; i < 4; i++) {
            pseudo.push_back(dst_ip[i]);
        }
        pseudo.push_back(0);
        pseudo.push_back(6);
        uint16_t tcpLen = tcp_header_len + (uint16_t)payload.size();
        pseudo.push_back(tcpLen >> 8 );
        pseudo.push_back(tcpLen &0xFF);
        pseudo.insert(pseudo.end(), packet.begin() + 20,packet.end());
        if (pseudo.size()% 2 != 0) {
            pseudo.push_back(0);
        }
        uint16_t tcp_csum = checksum((uint16_t*)pseudo.data(),pseudo.size() / 2);
        packet[36] = tcp_csum & 0xFF;
        packet[37] = tcp_csum >> 8;


        return packet;
    }
    std::vector<uint8_t> packetProcessor::buildUDPPacket(const std::string &src_ip, uint16_t src_port, const std::string &dst_ip, uint16_t dst_port, std::span<const uint8_t> payload) {
        unsigned int s[4], d[4];
        sscanf(src_ip.c_str(), "%u.%u.%u.%u", &s[0], &s[1], &s[2], &s[3]);
        sscanf(dst_ip.c_str(), "%u.%u.%u.%u", &d[0], &d[1], &d[2], &d[3]);
        uint8_t s_ip[4] = { (uint8_t)s[0], (uint8_t)s[1], (uint8_t)s[2], (uint8_t)s[3] };
        uint8_t d_ip[4] = { (uint8_t)d[0], (uint8_t)d[1], (uint8_t)d[2], (uint8_t)d[3] };

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
                session_.complete_receive(packet);
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
            size_t tcp_header_len = ((packet[ip_header_len + 12] >> 4) & 0x0F) * 4;
            size_t payload_len = total_len - ip_header_len - tcp_header_len;
            uint32_t seq_num = (packet[ip_header_len + 4 ]<< 24)| (packet[ip_header_len + 5] << 16) | (packet[ip_header_len + 6] << 8) | packet[ip_header_len + 7];
            bool syn = tcp_flags & 0x02;
            bool ack = tcp_flags & 0x10;
            bool fin = tcp_flags & 0x01;

            if (syn && !ack) {
                if (stream->state == tcpState::SynReceived || stream->connected) {
                    spdlog::info("TCP SYN retransmission {}:{} -> {}:{}", src_ip, src_port, dst_ip, dst_port);
                    auto syn_ack = buildTCPPacket(key, 0x12, stream->vpn_seq - 1, stream->client_seq, {});
                    session_.send_packet(syn_ack);
                    return;
                }
                if (stream->connecting) {
                    return;
                }
                stream->connecting = true;
                spdlog::info("TCP SYN {}:{} -> {}:{}", src_ip, src_port, dst_ip, dst_port);
                std::thread([this, key, dst_ip, dst_port, seq_num]() {
                    auto socks = std::make_unique<socks5Client>(socks_host_, socks_port_);
                    bool ok = socks->connect(dst_ip, dst_port);
                    std::lock_guard<std::mutex> lock(streams_mutex_);
                    auto& st = streams_[key];
                    if (st) {
                        st->connecting = false;
                        if (ok) {
                            st->socks = std::move(socks);
                            st->connected = true;
                            st->client_seq = seq_num + 1;
                            st->vpn_seq = 1000;
                            st->state = tcpState::SynReceived;
                            auto syn_ack = buildTCPPacket(key, 0x12, st->vpn_seq, st->client_seq, {});
                            spdlog::info("SYN-ACK built: {} bytes, src={}:{} dst={}:{} seq={} ack={}",
                                syn_ack.size(), key.remote_ip, key.remote_port,
                                key.local_ip, key.local_port, st->vpn_seq, st->client_seq);
                            session_.send_packet(syn_ack);
                            st->vpn_seq++;
                            spdlog::info("successfully connected via SOCKS5 to {}:{}", dst_ip, dst_port);
                        } else {
                            spdlog::error("Failed to connect via SOCKS5: {}:{}", dst_ip, dst_port);
                        }
                    }
                }).detach();
            }
            else if (ack && payload_len == 0 && stream->state == tcpState::SynReceived) {
                stream->state = tcpState::Established;
                spdlog::info ("TCP Handshake complete for {}:{}", dst_ip, dst_port);
            }
            else if (payload_len > 0) {
                stream->client_seq = seq_num +payload_len;
                auto ack_pkt = buildTCPPacket(key, 0x10,stream->vpn_seq, stream->client_seq, {});
                session_.send_packet(ack_pkt);

                if (stream->connected){
                    auto payload = packet.subspan(ip_header_len + tcp_header_len, payload_len);
                    stream->socks->send_packet(payload);
                }
            }
            else if (fin) {
                stream->client_seq = seq_num + 1;
                stream->state = tcpState::Closed;
                auto fin_ack = buildTCPPacket(key, 0x11, stream->vpn_seq,stream->client_seq,{});
                session_.send_packet(fin_ack);
                stream->vpn_seq++;
            }
        }
        else if (protocol == 17 ) {
            size_t udp_header_len = 8;
            if (packet.size() < ip_header_len + udp_header_len) return;

            uint16_t src_port = (packet[ip_header_len] << 8) | packet[ip_header_len  + 1];
            uint16_t dst_port = (packet[ip_header_len + 2] << 8)| packet[ip_header_len + 3];
            uint16_t udpLen = (packet[ip_header_len + 4] << 8) | packet[ip_header_len + 5];
            if (dst_port == 53) {
                size_t payloadLen = udpLen - 8;
                if (ip_header_len + 8 + payloadLen <= packet.size()) {
                    std::vector<uint8_t> dnsPayload(packet.begin() + ip_header_len + 8, packet.begin() + ip_header_len + 8 + payloadLen);
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
                std::lock_guard<std::mutex> lock(streams_mutex_);
                auto& stream = streams_[key];
                if (stream) {
                    const size_t MAX_SEG = 1400;
                    size_t offset = 0;
                    while (offset < payload.size()) {
                        size_t chunk = std::min(MAX_SEG, payload.size() - offset);
                        std::vector<uint8_t> seg(payload.begin() + offset, payload.begin() + offset + chunk);
                        uint8_t flags = 0x10;
                        if (offset + chunk >= payload.size()) {
                            flags = 0x18;
                        }
                        auto packet = buildTCPPacket(key, flags, stream->vpn_seq, stream->client_seq, seg);
                        session_.send_packet(packet);
                        stream->vpn_seq += chunk;
                        offset += chunk;
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

} // namespace ivpn::core