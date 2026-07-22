//
// Created by xint2 on 20/07/2026.
//
#include "../../include/core/bootstrap_waiter.h"
#include <spdlog/spdlog.h>

namespace ivpn::core {
    bootstrapWaiter::bootstrapWaiter(ivpn::tor::controlPort &tor): tor_(tor) {

    }
    bool bootstrapWaiter::wait(std::chrono::milliseconds timeout) {
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < timeout) {
            if (is_bootstrap_complete()) {
                spdlog::info("Tor bootstrap complete");
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        spdlog::error("Tor bootstrap timeout");
        return false;
    }
    bool bootstrapWaiter::is_bootstrap_complete() const {
        auto response = tor_.send("GETINFO status/bootstrap-phase");
        if (!response) {
            spdlog::debug("No resp");
            return false;
        }
        spdlog::debug("Tor responce : {}" , *response);

        auto pos =response->find("PROGRESS=");
        if (pos == std::string::npos) return false;
        int progress = 0;
        sscanf(response->substr(pos + 9).c_str(), "%d", &progress);
        return progress >= 100;
    }




}