//
// Created by xint2 on 20/07/2026.
//
#include "../../include/core/route_manager.h"
#include <spdlog//fmt/fmt.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <iphlpapi.h>
#include <spdlog/spdlog.h>
#include <fmt/format.h>

namespace ivpn::core {
    routeManager::~routeManager() {
        stop_monitoring();
    }
    std::string routeManager::getDefaultGateway() {
        MIB_IPFORWARDROW route;
        if (GetBestRoute(inet_addr("8.8.8.8"), 0, &route) ==NO_ERROR) {
            struct in_addr gw_addr;
            gw_addr.S_un.S_addr = route.dwForwardNextHop;
            return inet_ntoa(gw_addr);
        }
        return "";
    }
    void routeManager::start_monitoring(DWORD tor_pid) {
        if (monitoring_) {
            return;
        }
        tor_pid_ = tor_pid;
        gateway_ip_ = getDefaultGateway();
        if (gateway_ip_.empty()) {
            spdlog::error("Failed to find default physical gateway");
            return;
        }
        monitoring_= true;
        monitor_thread_=std::thread(&routeManager::monitorLoop, this);
        spdlog::info("Started dynamic route monitor for Tor PID: {} via Gateway: {}", tor_pid_,gateway_ip_);
    }

    void routeManager::stop_monitoring() {
        if (!monitoring_) return;
        monitoring_ = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
        for (const auto& ip:bypassed_ips_) {
            std::string cmd = fmt::format("route delete {}", ip);
            system((cmd + " > nul 2>&1").c_str());
        }
        bypassed_ips_.clear();
        spdlog::info("Stopped dynamic route monitor");
    }

    bool routeManager::addBypassRoute(const std::string &ip) {
        if (bypassed_ips_.contains(ip)) return true;
        spdlog::info("bypassing VPN for tor guard node: {}", ip);
        std::string  cmd = fmt::format("route add {} mask 255.255.255.255 {}",ip,gateway_ip_);
        int res = system((cmd + " >nul 2>&1").c_str());
        if (res == 0) {
            bypassed_ips_.insert(ip);
            return true;
        }
        return false;
    }
    void routeManager::monitorLoop() {
        while (monitoring_) {
            DWORD size = 0;
            GetExtendedTcpTable(NULL, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
            if (size > 0) {
                std::vector<BYTE> buffer(size);
                if (GetExtendedTcpTable(buffer.data(), &size,FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0)== NO_ERROR) {
                    PMIB_TCPTABLE_OWNER_PID pTcpTable = reinterpret_cast<PMIB_TCPTABLE_OWNER_PID>(buffer.data());
                    for (DWORD i = 0; i<pTcpTable->dwNumEntries; i++) {
                        if (pTcpTable->table[i].dwOwningPid == tor_pid_ && pTcpTable->table[i].dwState ==MIB_TCP_STATE_ESTAB) {
                            struct in_addr ip_addr;
                            ip_addr.S_un.S_addr = pTcpTable->table[i].dwRemoteAddr;
                            char ip_str[INET_ADDRSTRLEN];
                            inet_ntop(AF_INET, & ip_addr, ip_str, INET_ADDRSTRLEN);
                            std::string remote_ip(ip_str);
                            if (remote_ip!= "127.0.0.1" && remote_ip != "0.0.0.0") {
                                addBypassRoute(remote_ip);
                            }
                        }
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    bool routeManager::add_route(uint32_t destination, uint32_t mask, uint32_t gateway) {
        MIB_IPFORWARDROW row{};
        row.dwForwardDest = destination;
        row.dwForwardMask = mask;
        row.dwForwardNextHop = gateway;
        row.dwForwardIfIndex =0;
        row.dwForwardType = MIB_IPROUTE_TYPE_INDIRECT;
        row.dwForwardProto = MIB_IPPROTO_NETMGMT;
        row.dwForwardAge =0;

        DWORD result = CreateIpForwardEntry(&row);
        if (result != NO_ERROR) {
            spdlog::error("Failed to add route: {}", result);
            return false;
        }
        auto format_ip = [](unsigned long ip) {
            return fmt::format("{}.{}.{}.{}",
                (ip & 0xFF),
                ((ip >> 8) & 0xFF),
                ((ip >> 16) & 0xFF),
                ((ip >> 24) & 0xFF));
        };
        spdlog::info("Added route {} via {}", format_ip(destination), format_ip(gateway));
        return true;
    }
    bool routeManager::remove_route(uint32_t destination, uint32_t mask) {
        MIB_IPFORWARDROW row{};
        row.dwForwardDest = destination;
        row.dwForwardMask = mask;

        DWORD result = DeleteIpForwardEntry(&row);
        if (result != NO_ERROR) {
            spdlog::error("Failed to remove route: {}", result);
            return false;
        }
        spdlog::info("removed route {}", destination);
        return true;
    }
    bool routeManager::add_default_routes(uint32_t tun_gateway) {
        tun_gateway_ = tun_gateway;
        add_route(0, 0x80000000, tun_gateway);// 0.0.0.0/1
        add_route(0x80000000, 0x80000000, tun_gateway); //128.0.0.0/1
        spdlog::info("Added default routes via TUN gateway");
        return true;
    }
    bool routeManager::remove_default_route() {
        remove_route(0, 0x80000000);
        remove_route(0x80000000, 0x80000000);
        tun_gateway_ = 0 ;
        return true;
    }





}