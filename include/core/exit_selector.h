//
// Created by xint2 on 17/07/2026.
//

#ifndef IVPN_EXIT_SELECTOR_H
#define IVPN_EXIT_SELECTOR_H

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

#include "config.h"
#include "../tor/control_port.h"

using namespace ivpn::tor;
namespace ivpn::core {
    struct exitNode {
        std::string fingerprint;
        std::string ip;
        std::string city;
        uint64_t bandwidth = 0;

    };
    class exitSelector {
    public:
        exitSelector(class GeoIP& geoip, class controlPort& tor);
        std::vector<exitNode> get_exits_for_city(const std::string& city);
        std::vector<std::string> get_available_cities();
    private:
        GeoIP& geoip_;
        controlPort& tor_;
    };
}
#endif //IVPN_EXIT_SELECTOR_H