//
// Created by xint2 on 17/07/2026.
//
#include "../../include/core/exit_selector.h"
#include "../../include/tor/control_port.h"
#include "../../include/core/geoip.h"
#include <spdlog/spdlog.h>
#include <sstream>
#include <vector>
#include <algorithm>

using namespace ivpn::core;

exitSelector::exitSelector(GeoIP &geoip, controlPort &tor) : geoip_(geoip), tor_(tor) {
}

std::vector<std::string> exitSelector::get_available_countries() {
    auto response = tor_.send("GETINFO ns/all");
    if (!response) return {};
    spdlog::info("Fetched tor count.size: {} bytes", response->size());
    std::vector<std::string> countries;
    std::istringstream stream(*response);
    std::string line;
    std::string current_ip;

    while (std::getline(stream, line)) {
        if (line.find("r ") == 0) {
            std::istringstream ls(line);
            std::vector<std::string> tokens;
            std::string token;
            while (ls >> token) tokens.push_back(token);
            if (tokens.size() >= 7) {
                current_ip = tokens[tokens.size() - 3];
            }
            else {
                current_ip = "";
            }
        }
        else if (line.find("s ")== 0 && !current_ip.empty()) {
            if (line.find("Exit") == std::string::npos) continue;
            auto loc = geoip_.lookup(current_ip);
            if (!loc || loc->country.empty()) continue;
            auto it = std::find(countries.begin(), countries.end(), loc->country);
            if (it == countries.end()) {
                countries.push_back(loc->country);
            }
            current_ip = "";
        }
    }
    std::sort(countries.begin(), countries.end());
    return countries;
}

std::vector<exitNode> exitSelector::get_exits_for_country(const std::string &country) {
    auto response = tor_.send("GETINFO ns/all");
    if (!response) return {};

    std::vector<exitNode> exits;
    std::istringstream stream(*response);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.find("r ") != 0) continue;

        std::istringstream ls(line);
        std::vector<std::string> tokens;
        std::string token;
        while (ls >> token) tokens.push_back(token);

        if (tokens.size() < 7) continue;

        std::string identity = tokens[2];
        std::string ip = tokens[tokens.size() - 3];

        std::string flags_line;
        if (!std::getline(stream, flags_line)) continue;
        if (flags_line.find("s ") != 0) continue;
        if (flags_line.find("Exit") == std::string::npos) continue;

        auto loc = geoip_.lookup(ip);
        if (!loc) continue;

        spdlog::debug("IP {} -> country: {}", ip, loc->country);

        if (loc->country != country) continue;

        exitNode node;
        node.ip = ip;
        node.city = loc->city;
        node.bandwidth = 1000;
        node.fingerprint = identity;

        std::string data_line;
        while (std::getline(stream, data_line)) {
            if (data_line == ".") break;

            if (data_line.find("fingerprint ") == 0) {
                std::string fp = data_line.substr(12);
                fp.erase(std::remove(fp.begin(), fp.end(), ' '), fp.end());
                if (!fp.empty() && fp[0] == '$') fp = fp.substr(1);
                node.fingerprint = fp;
            }
            else if (data_line.find("w Bandwidth=") == 0) {
                try {
                    node.bandwidth = std::stoull(data_line.substr(12));
                } catch (...) {}
            }
        }

        spdlog::info("US Exit found: {} -> {}", ip, node.fingerprint.substr(0, 16) + "...");
        exits.push_back(node);
    }

    spdlog::info("Found {} exits for country {}", exits.size(), country);
    return exits;
}