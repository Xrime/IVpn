
//
// Created by xint2 on 15/07/2026.
//
#include "../../include/core/config.h"
#include <fstream>
#include  <spdlog//spdlog.h>

std::optional<config> load_config(const std::string &path) {
    config cfg;
    std::ifstream file(path);
    if (!file) {
        spdlog::warn("config not found at {}, using defaults", path);
        return cfg;
    }
    try {
        nlohmann::json j;
        file >> j;
        cfg.tor_binary = j.value("tor_binary", cfg.tor_binary);
        cfg.data_dir = j.value("data_dir", cfg.data_dir);
        cfg.socks_port = j.value("socks_port", cfg.socks_port);
        cfg.control_port = j.value("control_port", cfg.control_port);
        cfg.dns_port = j.value("dns_port", cfg.dns_port);
        cfg.default_city = j.value("default_city", cfg.default_city);
        cfg.default_hops = j.value("default_hops", cfg.default_hops);
        cfg.geoip_db = j.value("geoip_db", cfg.geoip_db);
        spdlog::info("loaded config from {}", path);
        return cfg;
    }catch (const std::exception& e) {
        spdlog::error("Failed to parse config: {}", e.what());
        return std::nullopt;
    }
}
void save_config(const config &cfg, const std::string &path) {
    nlohmann::json j = {
        {"tor_binary", cfg.tor_binary},
        ("data_dir", cfg.data_dir),
        {"socks_port", cfg.socks_port},
        {"control_port", cfg.control_port},
        {"dns_port",cfg.dns_port},
        {"default_city", cfg.default_city},
        {"default_hops", cfg.default_hops},
        ("geoip_db", cfg.geoip_db)
    };
    std::ofstream file(path);
    file << j.dump(4);
}

