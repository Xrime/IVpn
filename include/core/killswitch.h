#ifndef IVPN_KILLSWITCH_H
#define IVPN_KILLSWITCH_H


#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include <string>

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

const GUID FALLBACK_FWPM_LAYER_ALE_AUTH_CONNECT_V4 = { 0xc38d57d1, 0x05a7, 0x4c33, { 0x90, 0x4f, 0x7f, 0xbc, 0xee, 0xe6, 0x0e, 0x82 } };
const GUID FALLBACK_FWPM_LAYER_ALE_AUTH_CONNECT_V6 = { 0x4a72393b, 0x319f, 0x44bc, { 0x84, 0xc3, 0xba, 0x54, 0xdc, 0xb3, 0xb6, 0xb4 } };
const GUID FALLBACK_FWPM_SUBLAYER_UNIVERSAL = { 0x02b489a5, 0x71cb, 0x402a, { 0x96, 0xce, 0xb1, 0x18, 0xb5, 0x3e, 0x7d, 0x36 } };

namespace ivpn::core {
    class killSwitch {
    public:
        killSwitch();
        ~killSwitch();

        bool enable();
        bool disable();
        bool is_active() const;
        bool add_tor_permit_rule(const std::string& tor_exe_path);

    private:
        bool openSession();
        void close_session();
        HANDLE engine_ = nullptr;
        UINT64 filter_id_ = 0;
        UINT64 filter_id_v6_ = 0;
        bool active_ = false;
    };
}
#endif