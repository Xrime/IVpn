//
// Created by xint2 on 20/07/2026.
//
#include "../../include/core/route_manager.h"
#include <vector>
#include <spdlog/spdlog.h>
#include <fmt/format.h>

DWORD get_wintun_if_index() {
    ULONG outBufLen = 15000;
    std::vector<BYTE> buffer(outBufLen);
    DWORD dwRetVal = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, (PIP_ADAPTER_ADDRESSES)buffer.data(), &outBufLen);
    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        buffer.resize(outBufLen);
        dwRetVal = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, (PIP_ADAPTER_ADDRESSES)buffer.data(), &outBufLen);
    }
    if (dwRetVal == NO_ERROR) {
        PIP_ADAPTER_ADDRESSES pCurrAddresses = (PIP_ADAPTER_ADDRESSES)buffer.data();
        while (pCurrAddresses) {
            if (pCurrAddresses->FriendlyName && wcscmp(pCurrAddresses->FriendlyName, L"IVpn") == 0) {
                return pCurrAddresses->IfIndex;
            }
            pCurrAddresses = pCurrAddresses->Next;
        }
    }
    return 0;
}
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
        in_addr dest_addr, mask_addr, gw_addr;
        dest_addr.s_addr = destination;
        mask_addr.s_addr = mask;
        gw_addr.s_addr = gateway;
        std::string dest_str = inet_ntoa(dest_addr);
        std::string mask_str = inet_ntoa(mask_addr);
        std::string gw_str = inet_ntoa(gw_addr);
        DWORD ifIndex = get_wintun_if_index();
        std::string cmd = fmt::format("route add {} mask {} {} IF {}", dest_str, mask_str, gw_str, ifIndex);
        spdlog::info("Executing route command: {}", cmd);
        int res = system(cmd.c_str());
        if (res != 0) {
            spdlog::error("Failed to add route via command (res: {})", res);
            return false;
        }
        return true;
    }

    bool routeManager::remove_route(uint32_t destination, uint32_t mask, uint32_t gateway) {
        in_addr dest_addr, mask_addr, gw_addr;
        dest_addr.s_addr = destination;
        mask_addr.s_addr = mask;
        gw_addr.s_addr = gateway;
        
        std::string dest_str = inet_ntoa(dest_addr);
        std::string mask_str = inet_ntoa(mask_addr);
        std::string gw_str = inet_ntoa(gw_addr);
        DWORD ifIndex = get_wintun_if_index();
        
        std::string cmd = fmt::format("route delete {} mask {} {} IF {}", dest_str, mask_str, gw_str, ifIndex);
        spdlog::info("Executing route command: {}", cmd);
        
        int res = system(cmd.c_str());
        if (res != 0) {
            spdlog::error("Failed to remove route via command (res: {})", res);
            return false;
        }
        return true;
    }
    bool routeManager::add_default_routes(uint32_t tun_gateway) {
        tun_gateway_ = tun_gateway;
        add_route(inet_addr("0.0.0.0"), inet_addr("128.0.0.0"), tun_gateway);
        add_route(inet_addr("128.0.0.0"), inet_addr("128.0.0.0"), tun_gateway);
        spdlog::info("Added default routes via TUN gateway");
        return true;
    }
    bool routeManager::remove_default_route() {
        remove_route(inet_addr("0.0.0.0"), inet_addr("128.0.0.0"), tun_gateway_);
        remove_route(inet_addr("128.0.0.0"), inet_addr("128.0.0.0"), tun_gateway_);
        tun_gateway_ = 0 ;
        return true;
    }





}