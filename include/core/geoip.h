//
// Created by xint2 on 17/07/2026.
//

#ifndef IVPN_GEOIP_H
#define IVPN_GEOIP_H
#include <string>
#include <optional>

namespace ivpn::core {
    struct geoLocation {
        std::string country;
        std::string city;
    };
    class GeoIP {
    public:
        explicit GeoIP(const std::string& db_path);
        ~GeoIP();
        std::optional<geoLocation> lookup(const std::string& ip);

    private:
        void* mmdb_ = nullptr;
        bool open_ = false;
    };
}
#endif //IVPN_GEOIP_H