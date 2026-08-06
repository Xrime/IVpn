#include <spdlog/spdlog.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <nlohmann/json.hpp>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib)
#include "include/tor/control_port.h"
#include "include/core/ipc_client.h"
#include "include/UI/tui.h"


using namespace ivpn::core;
using json = nlohmann::json;
struct ClientConfig{
    std::string last_city = "Auto";};
ClientConfig load_config() {
    ClientConfig cfg;
    std::ifstream f("client_config.json");
    if (f.is_open()) {
        try {
            json j;
            f >> j;
            if (j.contains("last_city")) cfg.last_city = j["last_city"];
        }catch (...) {

        }
    }
    return cfg;
}
void save_config(const  ClientConfig& cfg) {
    json j;
    j["last_city"] = cfg.last_city;
    std::ofstream f("client_config.json");
    if (f.is_open()) f<< j.dump(4);
}
bool verify_connection() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "1.1.1.1", &addr.sin_addr);
    DWORD timeout = 2000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

    bool ok = (connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0);
    closesocket(sock);
    WSACleanup();
    return ok;
}
int main() {
    spdlog::set_level(spdlog::level::err);
    ipcClient ipc;
    ivpn::UI::tui ui;
    ClientConfig config = load_config();
    bool connected = false;
    auto cities = ipc.get_cities();
    ui.set_cities(cities);
    ui.set_current_city(config.last_city);
    ui.on_connect =[&](){
        if (!connected) {
            std::thread([&]() {
                if (ipc.send_command("connect")) {
                    for (int i = 0; i< 20; i++) {
                        if (verify_connection()) {
                            connected = true;
                            ui.set_connected(true);
                            ui.redraw();
                            break;
                        }    // --- 3. Initial Boot ---
                            // Fetch the 50+ countries instantly from the daemon
                            std::cout << "\n\n  Waiting for IVpn Background Engine to securely Bootstrap the Tor Network...\n";
                            std::cout << "  (This can take 30-60 seconds on a fresh install)\n";

                            auto cities = ipc.get_cities();
                            while (cities.empty()) {
                                std::this_thread::sleep_for(std::chrono::seconds(2));
                                cities = ipc.get_cities();
                            }

                            ui.set_cities(cities);
                            ui.set_current_city(config.last_city);
                    }
                }
            }).detach();
        }
    };
    ui.on_disconnect = [&]() {
        if (connected) {
            ipc.send_command("disconnect");
            connected = false;
            ui.set_connected(false);
            ui.redraw();
        }
    };
    ui.on_change_city = [&](const std::string& city) {
        config.last_city = city;
        save_config(config);
        std::thread([&, city]() {
           ipc.send_command("change_city", city);
            if (connected) {
                ui.set_connected(false);
                ui.redraw();

                for (int i = 0; i<20; i++) {
                    if (verify_connection()) {
                        ui.set_connected(true);
                        ui.redraw();
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }
        }).detach();
    };
    ui.on_quit = [&]() {
        if (connected) ipc.send_command("disconnect");
        ui.stop();
    };
    ui.run();
    return 0;
}