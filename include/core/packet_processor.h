//
// Created by xint2 on 19/07/2026.
//

#ifndef IVPN_PACKET_PROCESSOR_H
#define IVPN_PACKET_PROCESSOR_H
#include <cstdint>
#include <span>
#include "../../include/core/wintun.h"
#include "../../include/core/socks5.h"
namespace ivpn::core {
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
    };
}
#endif //IVPN_PACKET_PROCESSOR_H