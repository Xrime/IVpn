//
// Created by xint2 on 15/07/2026.
//

#include "../../include/core/wintun.h"
#include <windows.h>
#include <spdlog/spdlog.h>

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
    received_packet_fn_ = (wintunReceivedPacketFn)(mod, "WintunReceivePacket");
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
