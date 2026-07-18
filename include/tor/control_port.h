//
// Created by xint2 on 17/07/2026.
//

#ifndef IVPN_CONTROL_PORT_H
#define IVPN_CONTROL_PORT_H
#include <cstdint>
#include <string>
#include <optional>

namespace ivpn::tor {
    class controlPort {
    public:
        controlPort(const std::string& host = "127.0.0.1", uint16_t port = 9051);
        ~controlPort();

        bool connect();
        bool authenticate();
        void disconnect();
        bool connected() const { return sock_ >= 0;}
        std::optional<std::string> send(const std::string& cmd);
    private:
        std::string host_;
        uint16_t port_;
        int sock_ = -1;


    };
}
#endif //IVPN_CONTROL_PORT_H