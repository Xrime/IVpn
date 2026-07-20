#include <iostream>
#include <spdlog/spdlog.h>

#include "include/core/bootstrap_waiter.h"
#include "include/core/wintun.h"
#include "include/core/config.h"
#include "include/tor/control_port.h"
#include "include/core/exit_selector.h"
#include "include/core/circuit_builder.h"
#include "include/core/socks5.h"
#include "include/core/packet_processor.h"
#include "include/core/tor_launcher.h"
#include "include/core/geoip.h"


using namespace ivpn::core;
using namespace ivpn::tor;
int main() {
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("IVpn starting up...");
    auto cfg = load_config("config.json");
    if (!cfg) return 1;
    torLauncher launcher(cfg->tor_binary, cfg->data_dir);
    if (!launcher.start(cfg->socks_port, cfg->control_port, cfg->dns_port)) {
        spdlog::error("failed to start tor");
        return 1;
    }
    controlPort tor("127.0.0.1", cfg->control_port);
    if (!tor.connect()) {
        spdlog::error("Tor control port not responding");
        return 1;
    }
    bootstrapWaiter waiter(tor);
    if (!waiter.wait()) {
        spdlog::error("Tor bootstrap failed");
        return 1;
    }
    if (!tor.authenticate()) {
        spdlog::error("Tor authentication failed");
        return 1;
    }
    spdlog::info(" city: {}, Hops: {}", cfg->default_city, cfg->default_hops);
    GeoIP geoip(cfg->geoip_db);
    exitSelector selector(geoip, tor);
    auto exits = selector.get_exits_for_city(cfg->default_city);
    if (exits.empty()) {
        spdlog::error("No exits found in {}", cfg->default_city);
        return 1;
    }
    circuitBuilder builder(tor);
    std::vector<std::string> path;
    if (cfg->default_hops ==2) {
        path = {"$guard_fingerprint", exits[0].fingerprint};
    }else {
        path = {"$guard_fingerprint", "$middle_fingerprint", exits[0].fingerprint};
    }
    builder.buildCircuit(0, path);

    Wintun wintun;
    wintun.load();
    // if (!wintun.load()) {
    //     spdlog::error("Wintun not available. Install WireGuard or copy wintun.dll");
    //     return  1;
    // }
    auto adapter = wintun.create_adapter(L"IVpn", L"IVpn");
    // if (!adapter) {
    //     spdlog::warn("Could not create adapter (need admin?). Trying open");
    //     adapter = wintun.open_adapter(L"IVpn");
    // }
    // if (!adapter) {
    //     spdlog::error("No adapter avaliable");
    //     return 1;
    // }
    auto session = adapter->start_session(0x400000);
    // if (!session) {
    //     spdlog::error("failed to start session ");
    //     return 1;
    // }
    // spdlog::info("Wintun adapter ready. Reading packek ");

    socks5Client socks("127.0.0.1", cfg->socks_port);
    packetProcessor processor(*session, socks);
    processor.start();
    spdlog::info("IVpn running. press Enter to exit");
    std::cin.get();
    processor.stop();
    launcher.stop();
    return 0;
    // for (int i = 0; i < 5; i++) {
    //     size_t size=0;
    //     auto pkt = session->receive_packet(1000, &size);
    //     if (size > 0) {
    //         spdlog::info("Got packet {} BYTES", size);
    //     }
    //
    // }
    // spdlog::info("Done");
    // return 0;
    //
    // // spdlog::info("Tor binary: {}", cfg->tor_binary);
    // // spdlog::info("Control port {}", cfg->control_port);
    // //
    // // spdlog::info("Default hops: {}", cfg->default_hops);
    //
    // return 0;
}
