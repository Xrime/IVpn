//
// Created by xint2 on 17/07/2026.
//
#include "../../include/core/exit_selector.h"
#include "../../include/tor/control_port.h"
#include "../../include/core/geoip.h"
#include <spdlog/spdlog.h>
#include <sstream>

ivpn::core::exitSelector::exitSelector(GeoIP &geoip, controlPort &tor) : geoip_(geoip), tor_(tor) {

}
std::vector<ivpn::core::exitNode> ivpn::core::exitSelector::get_exits_for_city(const std::string &city) {
    auto response = tor_.send("GETINFO ns/all");
    if (!response) return {};
    std::vector<exitNode> exits;
    std::istringstream stream(*response);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.find("router ")!=0)continue;
        std::istringstream ls(line);
        std::string token, nickname, ip;
        ls >>token >>nickname>> ip;
        if (line.find("w Bandwidth=") == std::string::npos) continue;
        auto loc = geoip_.lookup(ip);
        if (!loc || loc->city != city) continue;

        exitNode node;
        node.ip = ip;

        node.city = loc->city;
        size_t pos = line.find("w Bandwidth=");
        if (pos != std::string::npos) {
            std::string bw = line.substr(pos + 13);
            std::istringstream bwstream(bw);
            bwstream >> node.bandwidth;

        }
        exits.push_back(node);
    }
    std::sort(exits.begin(), exits.end(), [](const exitNode& a, const exitNode& b) {
       return a.bandwidth > b.bandwidth;
    });
    return exits;
}

