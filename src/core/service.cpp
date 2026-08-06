//
// Created by xint2 on 20/07/2026.
//

#include <memory>
#include <thread>
#include <atomic>
#include "../../include/core/bootstrap_waiter.h"
#include "../../include/core/wintun.h"
#include "../../include/core/config.h"
#include "../../include/tor/control_port.h"
#include "../../include/core/exit_selector.h"
#include "../../include/core/circuit_builder.h"
#include "../../include/core/packet_processor.h"
#include "../../include/core/tor_launcher.h"
#include "../../include/core/geoip.h"
#include "../../include/core/killswitch.h"
#include "../../include/core/route_manager.h"
#include "../../include/core/ipc_server.h"
#include "../../include/core/service.h"
#include <spdlog/spdlog.h>

using  namespace ivpn::core;
using namespace ivpn::tor;

#define SERVICE_NAME TEXT("IVpnDaemon")

SERVICE_STATUS gServiceStatus = {0};
SERVICE_STATUS_HANDLE gStatusHandle = NULL;
HANDLE gServiceStopEvent = INVALID_HANDLE_VALUE;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI ServiceCtrlHandler(DWORD ctrlcode);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);
BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType) {
    switch (ctrlType) {
        case CTRL_C_EVENT:
        case CTRL_CLOSE_EVENT:
            spdlog::info("Ctrl+C received, shutting down gracefully...");
            if (gServiceStopEvent != INVALID_HANDLE_VALUE) {
                SetEvent(gServiceStopEvent);
            }
            return TRUE;
        default:
            return FALSE;
    }
}
bool assign_adapter_dns(const std::string& adapter_name, const std::string& dns_ip) {
    spdlog::info("Assign DNS {} to adapter {}", dns_ip, adapter_name);
    std::string cmd = fmt::format("netsh interface ip set dns name=\"{}\" static {}", adapter_name, dns_ip);
    int result = system((cmd + " >nul 2>&1").c_str());
    return result == 0;
}

bool assign_adapter_ip(const std::string& adapter_name, const std::string& ip) {
    spdlog::info("Assigning IP {} to adapter {}", ip, adapter_name);
    std::string cmd = fmt::format("netsh interface ip set address name=\"{}\" static {} 255.255.255.0", adapter_name, ip);
    int result = system((cmd + " >nul 2>&1").c_str());
    return result == 0;
}

bool is_admin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin == TRUE;
}
bool install_service() {
    if (!is_admin()) {
        spdlog::critical("must be admin to install");
        return false;
    }
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    SC_HANDLE scm = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!scm) {
        spdlog::error("failed to open Service Control Manager");
        return  false;
    }
    SC_HANDLE svc = CreateServiceA(
        scm, "IVpnDaemon", "IVpn Background Service",
        SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, path, NULL, NULL, NULL, NULL, NULL
    );
    if (svc) {
        spdlog::info("Ivpn service installed");
        CloseServiceHandle(svc);
        CloseServiceHandle(scm);
        return true;
    }
    spdlog::error("failed to install service. error code: {}", GetLastError());
    CloseServiceHandle(scm);
    return false;
}
bool uninstall_service() {
    if (!is_admin()) {
        spdlog::critical("Must be Administrator to unistall the service!");
        return false;
    }
    SC_HANDLE scm = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!scm) return false;
    SC_HANDLE svc = OpenServiceA(scm, "IVpnDaemon", SERVICE_STOP | DELETE);

    if (!svc) {
        spdlog::error("service not found");
        CloseServiceHandle(scm);
        return false;
    }
    SERVICE_STATUS status;
    ControlService(svc, SERVICE_CONTROL_STOP, &status);
    bool deleted = DeleteService(svc);
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);

    if (deleted) {
        spdlog::info("Ivpn service uninstalled successfully");

    }
    else {
        spdlog::error("failed to uninstall service . Error code: {}",GetLastError());
    }
    return deleted;
}
int main(int argc, char** argv) {
    if (argc >1) {
        std::string arg = argv[1];
        if (arg == "--install") {
            return install_service() ? 0: 1;
        }
        if (arg == "--uninstall") {
            return uninstall_service() ? 0 :1;
        }
        if (arg == "--console") {
            if (!is_admin()) {
                spdlog::critical("FATAL: ivpn_daemon must be run as Administrator!");
                return 1;
            }
        }
        spdlog::info("Running in console ");
        gServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
        ServiceWorkerThread(NULL);
        if (gServiceStopEvent != INVALID_HANDLE_VALUE) {
            CloseHandle(gServiceStopEvent);
        }
        return 0;
    }
    SERVICE_TABLE_ENTRY ServiceTable[] = {
        {(LPSTR)SERVICE_NAME, (LPSERVICE_MAIN_FUNCTIONA)ServiceMain},
        {NULL, NULL}
    };
    if (StartServiceCtrlDispatcher(ServiceTable) == FALSE) {
        spdlog::error("SSCD failed. this exe must be run as win service, not a console app");
        return GetLastError();
    }
    return 0;
}
VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {
    gStatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);
    if (!gStatusHandle) {
        return;
    }

    gServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    gServiceStatus.dwCurrentState = SERVICE_RUNNING;
    gServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    SetServiceStatus(gStatusHandle, &gServiceStatus);

    gServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);
    WaitForSingleObject(gServiceStopEvent, INFINITE);
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(gServiceStopEvent);
    CloseHandle(hThread);
    gServiceStatus.dwControlsAccepted = 0;
    gServiceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(gStatusHandle, &gServiceStatus);
}

VOID WINAPI ServiceCtrlHandler(DWORD ctrlCode) {
    switch (ctrlCode) {
        case SERVICE_CONTROL_STOP:
            if (gServiceStatus.dwCurrentState != SERVICE_RUNNING) {
                break;
            }
            gServiceStatus.dwControlsAccepted = 0;
            gServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
            SetServiceStatus(gStatusHandle, &gServiceStatus);
            SetEvent(gServiceStopEvent);
            break;
        default:
            break;
    }
}
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam) {
    // Fix working directory: Windows Services start in System32 by default.
    // We need to set it to the folder containing our exe so relative paths work.
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH)) {
        std::string dir(exePath);
        dir = dir.substr(0, dir.find_last_of("\\/"));
        SetCurrentDirectoryA(dir.c_str());
        spdlog::info("Set working directory to: {}", dir);
    }
    spdlog::info("IVpn starting up as System");
    auto cfg =load_config("config.json");
    if (!cfg) return 1;

    torLauncher launcher(cfg->tor_binary, cfg->data_dir);
    if (!launcher.start(cfg->socks_port,cfg->control_port, cfg->dns_port)) return 1;
    controlPort tor("127.0.0.1", cfg->control_port, cfg->data_dir);
    tor.connect();
    tor.authenticate();
    bootstrapWaiter waiter(tor);
    waiter.wait(std::chrono::seconds(120));
    GeoIP geoip(cfg->geoip_db);
    exitSelector selector(geoip, tor);
    circuitBuilder builder(tor);
    auto exits = selector.get_exits_for_country(cfg->default_country);
    // if (!exits.empty()) builder.buildCircuit(0,{exits[0].fingerprint});
    Wintun wintun;
    if (!wintun.load()) {
        spdlog::error("Failed to load wintun.dll!");
        return 1;
    }

    auto adapter = wintun.create_adapter(L"IVpn", L"IVpn");
    if (!adapter) {
        spdlog::error("Failed to create Wintun adapter");
        launcher.stop();
        return 1;
    }

    assign_adapter_ip("IVpn", "10.0.0.2");
    assign_adapter_dns("IVpn", "10.0.0.2");


    auto session = adapter->start_session(0x400000);
    if (!session) {
        spdlog::error("Failed to start Wintun session");
        return 1;
    }

    spdlog::info("Wintun adapter successfully created!");
    packetProcessor processor(*session, "127.0.0.1", cfg->socks_port);
    killSwitch ks;
    routeManager rm;
    std::atomic<bool> connected{false};

    ipcServer ipc;
    ipc.setOnConnect([&]() {
       if (!connected) {
           spdlog::info("Received Connect command from UI.");
           ks.enable();
           ks.add_tor_permit_rule(cfg->tor_binary);
           processor.start();
           uint32_t tun_ip = inet_addr("10.0.0.1");
           rm.start_monitoring(launcher.get_pid());
           rm.add_default_routes(tun_ip);
           connected = true;
       }
    });

    ipc.setOnDisconnect([&]() {
       if (connected) {
           spdlog::info("Client disconnect command from UI.");
           rm.stop_monitoring();
           connected = false;
           rm.remove_default_route();
           processor.stop();
           ks.disable();

       }
    });
    ipc.setOnGetCites([&]() {
       spdlog::info("IPC client request avaliable country.");
        return selector.get_available_countries();
    });
    // Build country name -> code map (e.g. "United States" -> "us")
    auto country_codes = selector.get_country_codes();
    spdlog::info("Built country code map with {} entries", country_codes.size());

    ipc.setOnChangeCity([&, country_codes](const std::string& country) {
       spdlog::info("Received ChangeCity command to: {}", country);
        // Look up the ISO country code for Tor
        auto it = country_codes.find(country);
        if (it == country_codes.end()) {
            spdlog::error("Unknown country: {}", country);
            return;
        }
        std::string code = it->second;
        std::string setconf_cmd = fmt::format("SETCONF ExitNodes={{{}}}", code);
        spdlog::info("Sending to Tor: {}", setconf_cmd);
        auto conf_response = tor.send(setconf_cmd);
        if (conf_response && conf_response->find("250 OK") != std::string::npos) {
            spdlog::info("Successfully set Tor exit node to {} ({})", country, code);
            tor.send("SETCONF StrictNodes=1");
            tor.send("SIGNAL NEWNYM");
            spdlog::info("Sent NEWNYM signal, old Tor circuit dropped");
        } else {
            spdlog::error("Failed to set Tor exit node. Response: {}", conf_response.value_or("Timeout"));
        }
    });
    ipc.start();
    WaitForSingleObject(gServiceStopEvent,INFINITE);

    ipc.stop();
    if (connected) {
        rm.remove_default_route();
        processor.stop();
        ks.disable();
    }
    launcher.stop();
    return 0;

}



