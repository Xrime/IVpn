//
// Created by xint2 on 15/07/2026.
//

#ifndef IVPN_CONFIG_H
#define IVPN_CONFIG_H
#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace ivpn::core {
    struct config {
        std::string tor_binary = "tor.exe";
        std::string data_dir = "./data/tor";
        uint16_t socks_port = 9050;
        uint16_t control_port = 9051;
        uint16_t dns_port = 9053;
        std::string default_city = "New York";
        int default_hops = 3;
        std::string geoip_db = "./geoip/GeoLite2-City.memb";

    };
    std::optional<config> load_config(const std::string& path);
    void save_config(const config& cfg, const std::string& path);
}
#endif //IVPN_CONFIG_H