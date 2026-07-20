//
// Created by xint2 on 20/07/2026.
//
#include "../../include/core/route_manager.h"
#include <windows.h>
#include <iphlpapi.h>
#include <spdlog/spdlog.h>
#include <fmt/format.h>

namespace ivpn::core {
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