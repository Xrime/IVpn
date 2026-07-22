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
#include "include/UI/tui.h"


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
    controlPort tor("127.0.0.1", cfg->control_port, cfg->data_dir);
    if (!tor.connect()) {
        spdlog::error("Tor control port not responding");
        return 1;
    }

    if (!tor.authenticate()) {
        spdlog::error("Tor authentication failed");
        return 1;
    }
    bootstrapWaiter waiter(tor);
    if (!waiter.wait()) {
        spdlog::error("Tor bootstrap failed");
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
    auto adapter = wintun.create_adapter(L"IVpn", L"IVpn");
    auto session = adapter->start_session(0x400000);
    socks5Client socks("127.0.0.1", cfg->socks_port);
    packetProcessor processor(*session, socks);

    ivpn::UI::tui ui;
    bool connected = false;
    while (true) {
        auto option = ui.show_menu(cfg->default_city, cfg->default_hops);
        switch (option) {
        case ivpn::UI::tui::menu_option::connect:
            processor.start();
            connected =true;
            ui.show_message("Connected to "+ cfg->default_city);
            break;
        case ivpn::UI::tui::menu_option::disconnect:
                processor.stop();
                connected =false;
                ui.show_message("Discooected");
                break;
        case ivpn::UI::tui::menu_option::change_city: {
            GeoIP geoip(cfg->geoip_db);
            exitSelector selector(geoip, tor);
            auto exits = selector.get_available_cities();
            ui.show_message("Available cities: " + exits[0]);
            break;
        }

        case ivpn::UI::tui::menu_option::list_cities:
            break;

        case ivpn::UI::tui::menu_option::exit_app:
            processor.stop();
            launcher.stop();
            return 0;

        }
    }
}
