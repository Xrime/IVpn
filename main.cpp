#include <spdlog/spdlog.h>
#include "include/tor/control_port.h"
#include "include/core/ipc_client.h"
#include "include/UI/tui.h"


using namespace ivpn::core;
using namespace ivpn::tor;
int main(){
    spdlog::set_level(spdlog::level::info);
    spdlog::info("IVpn starting up...");
    ipcClient ipc;
    ivpn::UI::tui ui;
    bool connected = false;
    std::string currentCountry = "US";
    int default_hop = 3;
    while (true) {
        auto option = ui.show_menu(currentCountry, default_hop, connected);
        switch (option) {
        case ivpn::UI::tui::menu_option::connect:
            if (!connected) {
                if (ipc.send_command("connect")) {
                    connected = true;
                    ui.show_message("Command sent! traffic is tunneling ");
                }else {
                    ui.show_message("Error: Failed to reach IVpn Deamon. service is not running");

                }
            }break;
        case ivpn::UI::tui::menu_option::disconnect:
            if (connected) {
                if (ipc.send_command("disconnect")) {
                    connected = false;
                    ui.show_message("Disconnected");
                }
            }
            break;
        case ivpn::UI::tui::menu_option::change_city: {
            ipc.send_command("change_city", "Berlin");//i must remove this and
            ui.show_message("Command sent, Requesting new exit node in berlin..");
            break;
        }
        case ivpn::UI::tui::menu_option::list_cities:
            ui.show_message("City lists.");
            break;

        case ivpn::UI::tui::menu_option::exit_app:
            return 0;

        }
    }
}