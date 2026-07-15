#include <iostream>
#include <spdlog/spdlog.h>

#include "include/core/config.h"

int main() {
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("IVpn starting up...");
    auto cfg = load_config("config.json");
    if (!cfg) return 1;

    spdlog::info("Tor binary: {}", cfg->tor_binary);
    spdlog::info("Control port {}", cfg->control_port);
    spdlog::info("Default city: {}", cfg->default_city);
    spdlog::info("Default hops: {}", cfg->default_hops);

    return 0;
}
