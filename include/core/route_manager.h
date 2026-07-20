//
// Created by xint2 on 20/07/2026.
//

#ifndef IVPN_ROUTE_MANAGER_H
#define IVPN_ROUTE_MANAGER_H
#include <string>
#include <cstdint>
namespace ivpn::core {
    class routeManager {
    public:
        routeManager() = default;
        bool add_route(uint32_t destination,uint32_t mask, uint32_t gateway);
        bool remove_route(uint32_t destination, uint32_t mask );
        bool add_default_routes(uint32_t tun_gateway);
        bool remove_default_route();

    private:
        uint32_t tun_gateway_= 0;
    };
}
#endif //IVPN_ROUTE_MANAGER_H