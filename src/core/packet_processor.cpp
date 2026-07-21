//
// Created by xint2 on 19/07/2026.
//
#include "../../include/core/packet_processor.h"
#include <spdlog/spdlog.h>
#include <thread>

namespace ivpn::core {
    packetProcessor::packetProcessor(WintunSession &session, socks5Client& socks) : session_(session), socks_(socks){

    }
    void packetProcessor::start() {
        running_ = true;
        std::thread(&packetProcessor::process_loop,this).detach();
    }
    void packetProcessor::stop() {
        running_ = false;
    }
    void packetProcessor::process_loop() {
        while (running_) {
            size_t size = 0;
            auto packet = session_.receive_packet(1000, &size);
            if (size>0 && !packet.empty()) {
                handle_ipv4_packet(packet);
            }
        }
    }
    void packetProcessor::handle_ipv4_packet(std::span<const uint8_t> packet) {
        if (packet.size() < 20) return;
        uint8_t version = packet[0] >> 4;
        // uint8_t ihl = packet[0] & 0x0F;
        if (version != 4) {
            return;
        }
        uint8_t protocol = packet[9];
        // uint16_t total_len = (packet[2] << 8) | packet[3];

        if (protocol == 6) {//tcp
            std::string src_ip = fmt::format("{}.{}.{}.{}",
                packet[12], packet[13], packet[14], packet[15]);
            std::string dst_ip = fmt::format("{}.{}.{}.{}", packet[16],packet[17], packet[18], packet[19]);
            uint16_t src_port = (packet[20+0]<< 8) | packet[20+1];
            uint16_t dst_port =(packet[20+2]<<8) | packet[20 +3];
            tcpStreamKey key{src_ip,dst_ip,src_port,dst_port};

            auto& stream = streams_[key];
            if (!stream.connected) {
                stream.connected = stream.socks.connect(dst_ip, dst_port);
                spdlog::info("TCP {}:{} -> {}:{}", src_ip,src_port,dst_ip,dst_port);
            }
            uint8_t tcp_offset = 20;
            uint8_t tcp_flags = packet[33];
            bool syn = tcp_flags & 0x02;
            bool ack = tcp_flags & 0x10;

            if (syn && !ack) {
                spdlog::info("TCP SYN {}:{} ->{}:{}", src_ip, src_port, dst_ip,dst_port );

            }
            size_t ip_header_len = (packet[0] & 0x0F)* 4;
            size_t tcp_header_len = ((packet[32] >> 4) & 0x0F) * 4;
            size_t total_len = (packet[2] << 8) | packet[3];
            size_t payload_len = total_len -ip_header_len - tcp_header_len;

            if (payload_len > 0) {
                auto payload = packet.subspan(ip_header_len + tcp_header_len , payload_len);
                if (stream.socks.send_packet(payload)) {
                    spdlog::debug("Forwarded {} bytes TCP ", payload_len);
                }
            }
        }else if (protocol ==17) {//udp
            spdlog::debug("UDP packet (dropping)");
        }
    }




}