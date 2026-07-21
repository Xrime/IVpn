//
// Created by xint2 on 21/07/2026.
//

#ifndef IVPN_KILLSWITCH_H
#define IVPN_KILLSWITCH_H

#include <cstdint>
#include <windows.h>
#include <fwpmu.h>
#include <guiddef.h>

namespace ivpn::core {
    class killSwitch {
    public:
        killSwitch();
        ~killSwitch();

        bool enable(uint16_t tor_socks_port = 9050);
        bool disable();
        bool is_active() const;

    private:
        bool openSession();
        void close_session();
        void add_filters();
        void remove_filter();

        HANDLE engine_ = nullptr;
        UINT64 filter_id_ = 0;
        bool active_ = false;
    };
}
#endif //IVPN_KILLSWITCH_H