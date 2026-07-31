//
// Created by xint2 on 15/07/2026.
//
#include "../../include/core/config.h"
#include <fstream>
#include <windows.h>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace ivpn::core {
    std::string get_exe_dir() {
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        return std::filesystem::path(buffer).parent_path().string();
    }
    std::optional<config> load_config(const std::string &path) {
        config cfg;
        std::string exe_dir = get_exe_dir();
        cfg.tor_binary = exe_dir+ "\\tor.exe";
        cfg.data_dir = exe_dir + "\\data\\tor";
        cfg.geoip_db = exe_dir + "\\geoip\\GeoLite2-City.mmdb";
        std::string full_path = exe_dir + "\\" + path;
        std::ifstream file(full_path);
        if (!file) {
            spdlog::warn("config not found at {}, using defaults", full_path);
            return cfg;
        }
        try {
            nlohmann::json j;
            file >> j;
            cfg.tor_binary     = j.value("tor_binary", cfg.tor_binary);
            cfg.data_dir       = j.value("data_dir", cfg.data_dir);
            cfg.geoip_db       = j.value("geoip_db", cfg.geoip_db);
            cfg.socks_port     = j.value("socks_port", cfg.socks_port);
            cfg.control_port   = j.value("control_port", cfg.control_port);
            cfg.dns_port       = j.value("dns_port", cfg.dns_port);
            cfg.default_country = j.value("default_country", cfg.default_country);
            cfg.default_hops   = j.value("default_hops", cfg.default_hops);
            spdlog::info("Loaded config from {}", full_path);
            return cfg;
        }catch (const std::exception& e) {
            spdlog::error("Failed to parse config: {}", e.what());
            return std::nullopt;
        }
    }
    void save_config(const config &cfg, const std::string &path) {
        nlohmann::json j ={
            {"tor_binary", cfg.tor_binary},
            {"data_dir", cfg.data_dir},
            {"socks_port", cfg.socks_port},
            {"control_port", cfg.control_port},
            {"dns_port", cfg.dns_port},
            {"default_country", cfg.default_country},
            {"default_hops", cfg.default_hops},
            {"geoip_db", cfg.geoip_db}
        };
        std::string full_path = get_exe_dir() + "\\" + path;
        std::ofstream file(full_path);
        file << j.dump(4);
    }
}