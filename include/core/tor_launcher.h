//
// Created by xint2 on 18/07/2026.
//

#ifndef IVPN_TOR_LAUNCHER_H
#define IVPN_TOR_LAUNCHER_H
#include <cstdint>
#include <string>
#include <optional>

class  torLauncher {
public:
    torLauncher(const std::string& tor_path, const std::string& data_dir);
    bool start(uint16_t socks_port = 9050,uint16_t control_port = 9051, uint16_t dns_port = 9053);
    bool stop();
    bool is_running() const;

private:
    std::string tor_path_;
    std::string data_dir_;
    void* process_handle_ = nullptr;
};
#endif //IVPN_TOR_LAUNCHER_H