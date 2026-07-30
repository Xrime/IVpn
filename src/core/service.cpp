//
// Created by xint2 on 20/07/2026.
//

#include <windows.h>
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

bool assign_adapter_ip(const std::string& adapter_name, const std::string& ip) {
    spdlog::info("Assigning IP {} to adapter {}", ip, adapter_name);
    std::string cmd = fmt::format("netsh interface ip set address name= \"{}\"static {} 255.255.255.0",adapter_name, ip );
    int result = system((cmd + " >nul 2>&1").c_str());
    return result ==0;
}

int main(int argc, char** argv) {
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
    spdlog::info("IVpn starting up as System");
    auto cfg =load_config("C:\\Program Files\\IVpn\\config.json");
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
    if (!exits.empty()) builder.buildCircuit(0,{exits[0].fingerprint});
    Wintun wintun;
    wintun.load();
    auto adapter = wintun.create_adapter(L"IVpn", L"IVpn");
    assign_adapter_ip("IVpn", "10.0.0.2");
    auto session = adapter->start_session(0x400000);
    packetProcessor processor(*session, "127.0.0.1", cfg->socks_port);
    killSwitch ks;
    routeManager rm;
    std::atomic<bool> connected{false};

    ipcServer ipc;
    ipc.setOnConnect([&]() {
       if (!connected) {
           spdlog::info("Received Connect command from UI.");
           ks.enable();
           processor.start();
           uint32_t tun_ip = inet_addr("10.0.0.2");
           rm.add_default_routes(tun_ip);
           connected = true;
       }
    });

    ipc.seOnDisconnect([&]() {
       if (connected) {
           spdlog::info("Received disconnect command from UI.");
           rm.remove_default_route();
           processor.stop();
           ks.disable();
           connected = false;
       }
    });
    ipc.setOnChangeCity([&](const std::string& city) {
       spdlog::info("Received ChangeCity command to: {}", city);
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



// SERVICE_STATUS_HANDLE service::status_handle_ = nullptr;
// SERVICE_STATUS service::status_ = {};
// service::service(const std::wstring &name) : name_(name) {

// }
// bool service::install() {
//     SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
//     if (!scm) {
//         spdlog:  :error("OpenSCManager failed: {}", GetLastError());
//         return false;
//     }
//     SC_HANDLE svc = CreateServiceW(
//         scm,(name_+ L"svc").c_str(),(name_ + L"Service").c_str(), SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START,SERVICE_ERROR_NORMAL,
//         nullptr,nullptr,nullptr,nullptr,nullptr,nullptr
//         );
//     if (!svc) {
//         bool already_exists = GetLastError() == ERROR_SERVICE_EXISTS;
//         spdlog::error("{} service install failed", already_exists ? "Service already exists" : "Failed to create");
//         CloseServiceHandle(scm);
//         return false;
//     }
//     CloseServiceHandle(svc);
//     CloseServiceHandle(scm);
//     spdlog::info("Service installed");
//     return true;
// }
//
// bool service::uninstall() {
//     SC_HANDLE scm = OpenSCManagerW(nullptr,nullptr, SC_MANAGER_CONNECT);
//     if (!scm) return false;
//     SC_HANDLE svc = OpenServiceW(scm, (name_ +  L"svc").c_str(), SERVICE_STOP | DELETE);
//     if (!svc) {
//         CloseServiceHandle(scm);
//         return false;
//     }
//     SERVICE_STATUS ss;
//     ControlService(svc, SERVICE_CONTROL_STOP, &ss);
//     bool ok = DeleteService(svc);
//     CloseServiceHandle(svc);
//     CloseServiceHandle(scm);
//     return ok;
// }
// bool service::start() {
//     SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
//     if (!scm) return false;
//
//     SC_HANDLE svc = OpenServiceW(scm, (name_ + L"svc").c_str(), SERVICE_START);
//     if (!svc) {
//         CloseServiceHandle(scm);
//         return false;
//     }
//     bool ok = StartServiceW(svc, 0, nullptr);
//     CloseServiceHandle(svc);
//     CloseServiceHandle(scm);
//     return ok;
// }
// bool service::stop() {
//     SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
//     if (!scm) return false;
//
//     SC_HANDLE svc = OpenServiceW(scm, (name_ + L"svc").c_str(), SERVICE_STOP);
//     if (!svc) {
//         CloseServiceHandle(scm);
//         return false;
//     }
//
//     SERVICE_STATUS ss;
//     bool ok = ControlService(svc, SERVICE_CONTROL_STOP, &ss);
//     CloseServiceHandle(svc);
//     CloseServiceHandle(scm);
//     return ok;
// }
// void WINAPI service::service_main(DWORD, LPTSTR*) {
//     status_handle_ = RegisterServiceCtrlHandlerW(L"ivpn", service_ctrl);
//     status_.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
//     status_.dwCurrentState = SERVICE_RUNNING;
//     status_.dwControlsAccepted = SERVICE_ACCEPT_STOP;
//     SetServiceStatus(status_handle_, &status_);
//     spdlog::info("IVpn service running");
// }
//
// void WINAPI service::service_ctrl(DWORD ctrl) {
//     switch (ctrl) {
//         case SERVICE_CONTROL_STOP:
//             status_.dwCurrentState = SERVICE_STOPPED;
//             break;
//     }
//     SetServiceStatus(status_handle_, &status_);
// }
//
//

