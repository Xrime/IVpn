//
// Created by xint2 on 20/07/2026.
//
#include "../../include/core/service.h"
#include <spdlog/spdlog.h>

namespace ivpn::core {
    SERVICE_STATUS_HANDLE service::status_handle_ = nullptr;
    SERVICE_STATUS service::status_ = {};
    service::service(const std::wstring &name) : name_(name) {

    }
    bool service::install() {
        SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
        if (!scm) {
            spdlog::error("OpenSCManager failed: {}", GetLastError());
            return false;
        }
        SC_HANDLE svc = CreateServiceW(
            scm,(name_+ L"svc").c_str(),(name_ + L"Service").c_str(), SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START,SERVICE_ERROR_NORMAL,
            nullptr,nullptr,nullptr,nullptr,nullptr,nullptr
            );
        if (!svc) {
            bool already_exists = GetLastError() == ERROR_SERVICE_EXISTS;
            spdlog::error("{} service install failed", already_exists ? "Service already exists" : "Failed to create");
            CloseServiceHandle(scm);
            return false;
        }
        CloseServiceHandle(svc);
        CloseServiceHandle(scm);
        spdlog::info("Service installed");
        return true;
    }

    bool service::uninstall() {
        SC_HANDLE scm = OpenSCManagerW(nullptr,nullptr, SC_MANAGER_CONNECT);
        if (!scm) return false;
        SC_HANDLE svc = OpenServiceW(scm, (name_ +  L"svc").c_str(), SERVICE_STOP | DELETE);
        if (!svc) {
            CloseServiceHandle(scm);
            return false;
        }
        SERVICE_STATUS ss;
        ControlService(svc, SERVICE_CONTROL_STOP, &ss);
        bool ok = DeleteService(svc);
        CloseServiceHandle(svc);
        CloseServiceHandle(scm);
        return ok;
    }
    bool service::start() {
        SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
        if (!scm) return false;

        SC_HANDLE svc = OpenServiceW(scm, (name_ + L"svc").c_str(), SERVICE_START);
        if (!svc) {
            CloseServiceHandle(scm);
            return false;
        }
        bool ok = StartServiceW(svc, 0, nullptr);
        CloseServiceHandle(svc);
        CloseServiceHandle(scm);
        return ok;
    }
    bool service::stop() {
        SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
        if (!scm) return false;

        SC_HANDLE svc = OpenServiceW(scm, (name_ + L"svc").c_str(), SERVICE_STOP);
        if (!svc) {
            CloseServiceHandle(scm);
            return false;
        }

        SERVICE_STATUS ss;
        bool ok = ControlService(svc, SERVICE_CONTROL_STOP, &ss);
        CloseServiceHandle(svc);
        CloseServiceHandle(scm);
        return ok;
    }
    void WINAPI service::service_main(DWORD, LPTSTR*) {
        status_handle_ = RegisterServiceCtrlHandlerW(L"ivpn", service_ctrl);
        status_.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
        status_.dwCurrentState = SERVICE_RUNNING;
        status_.dwControlsAccepted = SERVICE_ACCEPT_STOP;
        SetServiceStatus(status_handle_, &status_);
        spdlog::info("IVpn service running");
    }

    void WINAPI service::service_ctrl(DWORD ctrl) {
        switch (ctrl) {
            case SERVICE_CONTROL_STOP:
                status_.dwCurrentState = SERVICE_STOPPED;
                break;
        }
        SetServiceStatus(status_handle_, &status_);
    }






}