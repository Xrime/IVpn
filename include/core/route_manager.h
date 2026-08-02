//
// Created by xint2 on 20/07/2026.
//

#ifndef IVPN_ROUTE_MANAGER_H
#define IVPN_ROUTE_MANAGER_H
#include <string>
#include <thread>
#include <atomic>
#include <unordered_set>
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <cstdint>
namespace ivpn::core {
    class routeManager {
    public:
        routeManager() = default;
        ~routeManager();
        bool add_route(uint32_t destination,uint32_t mask, uint32_t gateway);
        bool remove_route(uint32_t destination, uint32_t mask, uint32_t gateway);
        bool add_default_routes(uint32_t tun_gateway);
        bool remove_default_route();
        void start_monitoring(DWORD tor_pid);
        void stop_monitoring();
    private:
        uint32_t tun_gateway_= 0;
        std::atomic<bool> monitoring_{false};
        std::thread monitor_thread_;
        DWORD tor_pid_ = 0;
        std::string gateway_ip_;
        std::unordered_set<std::string> bypassed_ips_;
        void monitorLoop();
        bool addBypassRoute(const std::string& ip);
        std::string getDefaultGateway();
    };
}
#endif //IVPN_ROUTE_MANAGER_H