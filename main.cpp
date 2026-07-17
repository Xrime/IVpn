#include <iostream>
#include <spdlog/spdlog.h>
#include "include/core/wintun.h"
#include "include/core/config.h"

int main() {
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("IVpn starting up...");
    auto cfg = load_config("config.json");
    if (!cfg) return 1;
    spdlog::info("Default city: {}", cfg->default_city);

    Wintun wintun;
    if (!wintun.load()) {
        spdlog::error("Wintun not available. Install WireGuard or copy wintun.dll");
        return  1;
    }
    auto adapter = wintun.create_adapter(L"IVpn", L"IVpn");
    if (!adapter) {
        spdlog::warn("Could not create adapter (need admin?). Trying open");
        adapter = wintun.open_adapter(L"IVpn");
    }
    if (!adapter) {
        spdlog::error("No adapter avaliable");
        return 1;
    }
    auto session = adapter->start_session(0x400000);
    if (!session) {
        spdlog::error("failed to start session ");
        return 1;
    }
    spdlog::info("Wintun adapter ready. Reading packek ");

    for (int i = 0; i < 5; i++) {
        size_t size=0;
        auto pkt = session->receive_packet(1000, &size);
        if (size > 0) {
            spdlog::info("Got packet {} BYTES", size);
        }

    }
    spdlog::info("Done");
    return 0;

    // spdlog::info("Tor binary: {}", cfg->tor_binary);
    // spdlog::info("Control port {}", cfg->control_port);
    //
    // spdlog::info("Default hops: {}", cfg->default_hops);

    return 0;
}
