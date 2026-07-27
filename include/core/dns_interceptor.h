//
// Created by xint2 on 27/07/2026.
//

#ifndef IVPN_DNS_INTERCEPTOR_H
#define IVPN_DNS_INTERCEPTOR_H
#include <cstdint>
#include <span>
#include <vector>

namespace ivpn::core {
    class dnsInterceptor {
    public:
        dnsInterceptor(uint16_t tor_dns_port = 9053);
        ~dnsInterceptor();

        std::vector<uint8_t> resolve(std::span<const uint8_t> query_payload);
    private:
        uint16_t tor_dns_port_;
        int sock_ = -1;
    };
}
#endif //IVPN_DNS_INTERCEPTOR_H