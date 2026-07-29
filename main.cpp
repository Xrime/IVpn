#include <iostream>
#include <spdlog/spdlog.h>
#include <windows.h>
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
#include "include/core/killswitch.h"
#include "include/core/route_manager.h"

using namespace ivpn::core;
using namespace ivpn::tor;
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
    if (!waiter.wait(std::chrono::seconds(120))) {
        spdlog::error("Tor bootstrap failed");
        return 1;
    }

    spdlog::info(" city: {}, Hops: {}", cfg->default_country, cfg->default_hops);
    GeoIP geoip(cfg->geoip_db);
    exitSelector selector(geoip, tor);
    auto countries = selector.get_available_countries();
    spdlog::info("Found {} countries: ", countries.size());
    for (const auto& c : countries) {
        spdlog::info(" - {}", c);
    }
    auto exits = selector.get_exits_for_country(cfg->default_country);
    if (exits.empty()) {
        spdlog::error("No exits found in {}", cfg->default_country);
        return 1;
    }

    // Use real exit fingerprints from Tor
    std::string exit_fingerprint = exits[0].fingerprint;
    spdlog::info("Using exit: {} ({})", exits[0].ip, exit_fingerprint.substr(0, 16) + "...");

    circuitBuilder builder(tor);
    std::vector<std::string> path;
    if (cfg->default_hops == 2) {
        // 2-hop: use exit directly (Tor will pick guard)
        path = {exit_fingerprint};
    } else {
        // 3-hop: need real guard and middle fingerprints
        // For now, use EXIT guard to let Tor pick appropriate nodes
        std::string guard_fp = exits.size() > 1 ? exits[1].fingerprint : exit_fingerprint;
        std::string middle_fp = exits.size() > 2 ? exits[2].fingerprint : exits[0].fingerprint;
        path = {guard_fp, middle_fp, exit_fingerprint};
    }
    builder.buildCircuit(0, path);

    Wintun wintun;
    wintun.load();
    auto adapter = wintun.create_adapter(L"IVpn", L"IVpn");
    auto session = adapter->start_session(0x400000);
    packetProcessor processor(*session, "127.0.0.1", cfg->socks_port);

    ivpn::UI::tui ui;
    bool connected = false;
    while (true) {
        auto option = ui.show_menu(cfg->default_country, cfg->default_hops);
        switch (option) {
        case ivpn::UI::tui::menu_option::connect:
            processor.start();
            connected = true;
            ui.show_message("Connected to " + cfg->default_country);
            break;
        case ivpn::UI::tui::menu_option::disconnect:
                processor.stop();
                connected = false;
                ui.show_message("Disconnected");
                break;
        case ivpn::UI::tui::menu_option::change_city: {
            GeoIP geoip(cfg->geoip_db);
            exitSelector selector(geoip, tor);
            auto countries = selector.get_available_countries();
            std::string msg = "Available countries (" + std::to_string(countries.size()) + "):\n";
            for (size_t i = 0; i < std::min(countries.size(), size_t(10)); i++) {
                msg += "  - " + countries[i] + "\n";
            }
            ui.show_message(msg);
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