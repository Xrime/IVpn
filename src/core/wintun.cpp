//
// Created by xint2 on 15/07/2026.
//

#include "../../include/core/wintun.h"
#include <windows.h>
#include <spdlog/spdlog.h>


namespace ivpn::core {
    Wintun::~Wintun() {
        if (module_) {
            FreeLibrary(static_cast<HMODULE>(module_));
        }
    }
    bool Wintun::load() {
        if (module_) {
            return true;
        }
        HMODULE mod = LoadLibraryW(L"wintun.dll");
        if (!mod) {
            spdlog::error("Failed to load wintun.dill: {}", GetLastError());
            return false;
        }
        module_ = mod;
        create_adapter_fn_ = (wintunCreateAdapterFn)GetProcAddress(mod, "wintunCreateAdapter");
        open_adapter_fn_ = (wintunOpenAdapterFn)GetProcAddress(mod, "WintunOpenAdapter");
        delete_adapter_fn_ = (wintunDeleteAdapterFn)GetProcAddress(mod, "WintunDeleteAdapter");
        start_session_fn_ = (wintunStartSessionFn)GetProcAddress(mod , "WintunStartSession");
        end_session_fn_ = (wintunEndSessionFn)GetProcAddress(mod, "wintunEndSession");
        received_packet_fn_ = (wintunReceivedPacketFn)GetProcAddress(mod, "WintunReceivePacket");
        send_packet_fn_ = (wintunSendPacketFn)GetProcAddress(mod, "WintunSendPacket");

        if (!create_adapter_fn_ || !open_adapter_fn_|| !delete_adapter_fn_ || !start_session_fn_|| !end_session_fn_||! received_packet_fn_|| !send_packet_fn_) {
            spdlog::error("Missing Wintun exports");
            FreeLibrary(mod);
            module_= nullptr;
            return false;
        }
        spdlog::info("Wintun loaded successfully");
        return true;
    }
    std::unique_ptr<WintunAdapter> Wintun::create_adapter(const std::wstring &name, const std::wstring &tunnel_type) {
        if (!loaded()) return nullptr;
        WintunAdapter* handle = create_adapter_fn_(name.c_str(), tunnel_type.c_str(), nullptr);
        if (!handle) {
            spdlog::error("WintunCreateAdapter failed: {}", GetLastError());
            return nullptr;
        }
        return std::make_unique<WintunAdapter>(this, handle);
    }

    std::unique_ptr<WintunAdapter> Wintun::open_adapter(const std::wstring &name) {
        if (!loaded()) return nullptr;
        WintunAdapter* handle = open_adapter_fn_(name.c_str());
        if (!handle) {
            spdlog::error("WintunOpenAdapter failed: {}", GetLastError());
            return nullptr;
        }
        return std::make_unique<WintunAdapter>(this, handle);
    }
    bool Wintun::delete_adapter(const std::wstring& name) {
        auto adapter = open_adapter(name);
        if (!adapter) {
            return false;
        }
        return delete_adapter_fn_(adapter->handle_);
    }
    WintunAdapter::~WintunAdapter() {
        if (handle_ && wintun_ && wintun_->delete_adapter_fn_) {
            wintun_->delete_adapter_fn_(handle_);
        }
    }
    std::unique_ptr<WintunSession> WintunAdapter::start_session(size_t capacity) {
        if (!wintun_ || !wintun_->start_session_fn_) return nullptr;
        WintunSession* handle = wintun_->start_session_fn_(handle_, capacity);
        if (!handle) {
            spdlog::error("WintunStartSession failed: {}", GetLastError());
            return nullptr;
        }
        return std::make_unique<WintunSession>(this, handle);
    }

    const wchar_t* WintunAdapter::name() const {
        // WintunAdapter has a Name method in newer versions, but we'll skip for now
        return L"";
    }

    WintunSession::~WintunSession() {
        if (handle_ && adapter_ && adapter_->wintun_ && adapter_->wintun_->end_session_fn_) {
            adapter_->wintun_->end_session_fn_(handle_);
        }
    }

    std::span<uint8_t> WintunSession::receive_packet(uint32_t timeout_ms, size_t *out_size) {
        if (!adapter_ || !adapter_->wintun_ || !adapter_->wintun_->received_packet_fn_) {
            *out_size = 0;
            return {};
        }
        size_t size =0;
        void* buf = adapter_->wintun_->received_packet_fn_(handle_, &size);
        if (!buf) {
            *out_size = 0;
            return {};
        }
        *out_size = size;
        return std::span<uint8_t>(static_cast<uint8_t*>(buf), size);

    }
    std::span<uint8_t> WintunSession::allocate_sd_packet(size_t size) {
        if (!adapter_ || !adapter_->wintun_ || !adapter_->wintun_->send_packet_fn_) {
            return {};
        }
        void* buf = adapter_->wintun_->send_packet_fn_(handle_, size);
        if (!buf) return {};
        return std::span<uint8_t>(static_cast<uint8_t*>(buf), size);

    }
    void WintunSession::complete_send(size_t size) {
        (void)size;
    }
}




