//
// Created by xint2 on 20/07/2026.
//

#ifndef IVPN_BOOTSTRAP_WAITER_H
#define IVPN_BOOTSTRAP_WAITER_H
#include <chrono>
#include <thread>
#include <../include/tor/control_port.h>

namespace ivpn::core {
    class bootstrapWaiter {
    public:
        explicit bootstrapWaiter(ivpn::tor::controlPort& tor);
        bool wait(std::chrono::milliseconds timeout = std::chrono::seconds(60));
    private:
        tor::controlPort& tor_;
        bool is_bootstrap_complete() const;
    };
}
#endif //IVPN_BOOTSTRAP_WAITER_H