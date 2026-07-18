//
// Created by xint2 on 17/07/2026.
//
#include "../../include/core/geoip.h"
#include <maxminddb.h>
#include <spdlog/spdlog.h>

namespace ivpn::core {
    GeoIP::GeoIP(const std::string &db_path) {
        MMDB_s mmdb;
        int status = MMDB_open(db_path.c_str(), MMDB_MODE_MMAP, & mmdb);
        if (status != MMDB_SUCCESS) {
            spdlog::error("Failed to open GeoIP DB: {}", db_path);
            return;
        }
        mmdb_ = new MMDB_s(mmdb);
        open_ = true;
        spdlog::info("GeoIP database loaded: {}", db_path);

    }
    GeoIP::~GeoIP() {
        if (open_) {
            MMDB_close(static_cast<MMDB_s *>(mmdb_));
        }
        delete static_cast<MMDB_s*>(mmdb_);
    }
    std::optional<geoLocation> GeoIP::lookup(const std::string &ip) {
        if (!open_) return {};
        MMDB_s* mmdb = static_cast<MMDB_s *>(mmdb_);

        int gai_error = 0;
        int status = 0;
        MMDB_lookup_result_s result =MMDB_lookup_string(mmdb, ip.c_str(), &gai_error, &status);

        if (gai_error != 0 || status != MMDB_SUCCESS || !result.found_entry) {
            return {};
        }
        geoLocation loc;
        MMDB_entry_data_s data;

        int country_status =MMDB_get_value(&result.entry, & data, "country", "names", "en", nullptr);
        if (country_status == MMDB_SUCCESS && data.has_data) {
            loc.country = std::string(data.utf8_string, data.data_size);

        }
        int city_status =MMDB_get_value(&result.entry, &data, "city", "name", "en", nullptr);
        if (city_status== MMDB_SUCCESS && data.has_data) {
            loc.city = std::string(data.utf8_string, data.data_size);
        }
        return loc;
    }



}
