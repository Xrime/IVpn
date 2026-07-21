//
// Created by xint2 on 20/07/2026.
//

#ifndef IVPN_SERVICE_H
#define IVPN_SERVICE_H
#include <string>
#include <windows.h>

namespace ivpn::core {
    class service {
    public:
        explicit service (const std::wstring& name);
        bool install();
        bool uninstall();
        bool start();
        bool stop();

    private:
        std::wstring name_;
        static SERVICE_STATUS_HANDLE status_handle_;
        static SERVICE_STATUS status_;
        static void WINAPI service_main(DWORD, LPTSTR*);
        static void WINAPI service_ctrl(DWORD);

    };
}
#endif //IVPN_SERVICE_H