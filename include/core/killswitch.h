#ifndef IVPN_KILLSWITCH_H
#define IVPN_KILLSWITCH_H


#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <initguid.h>
#include <fwpmu.h>
#include <fwpmtypes.h>
#include <cstdint>

#ifndef FWPM_SESSION_FLAG_DYNAMIC
#define FWPM_SESSION_FLAG_DYNAMIC 0x00000001
#endif

#ifndef FWPM_FILTER_FLAG_PERSISTENT
#define FWPM_FILTER_FLAG_PERSISTENT 0x00000001
#endif

const GUID FALLBACK_FWPM_LAYER_OUTBOUND_TRANSPORT_V4 = { 0x23a31bb0, 0xc1f3, 0x40d5, { 0xa6, 0xb8, 0xd0, 0x58, 0x22, 0xd8, 0xa1, 0x14 } };
const GUID FALLBACK_FWPM_SUBLAYER_UNIVERSAL = { 0x02b489a5, 0x71cb, 0x402a, { 0x96, 0xce, 0xb1, 0x18, 0xb5, 0x3e, 0x7d, 0x36 } };

namespace ivpn::core {
    class killSwitch {
    public:
        killSwitch();
        ~killSwitch();

        bool enable();
        bool disable();
        bool is_active() const;

    private:
        bool openSession();
        void close_session();
        HANDLE engine_ = nullptr;
        UINT64 filter_id_ = 0;
        bool active_ = false;
    };
}
#endif