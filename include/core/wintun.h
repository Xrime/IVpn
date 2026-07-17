//
// Created by xint2 on 15/07/2026.
//

#ifndef IVPN_WINTUN_H
#define IVPN_WINTUN_H
#include <cstdint>
#include <string>
#include <optional>
#include <span>
#include <memory>

struct WintunAdapter;
struct WintunSession;

class Wintun {
public:
    Wintun() = default;
    ~Wintun();
    bool load();
    bool loaded() const {return module_ != nullptr;}
    std::unique_ptr<WintunAdapter> create_adapter(const std::wstring& name,const std::wstring& tunnel_type);
    std::unique_ptr<WintunAdapter> open_adapter(const std::wstring& name);
    bool delete_adapter(const std::wstring& name);

private:
    void* module_= nullptr;
    using wintunCreateAdapterFn = WintunAdapter*(*)(const wchar_t*, const wchar_t*, void*);
    using wintunOpenAdapterFn = WintunAdapter*(*)(const wchar_t*);
    using wintunDeleteAdapterFn =bool(*)(WintunAdapter*);
    using wintunStartSessionFn = WintunSession*(*)(WintunAdapter*, size_t);
    using wintunEndSessionFn = void(*)(WintunSession*);
    using wintunReceivedPacketFn =void*(*)(WintunSession*, size_t*z);
    using wintunSendPacketFn = void*(*)(WintunSession*, size_t);
    wintunCreateAdapterFn create_adapter_fn_ = nullptr;
    wintunOpenAdapterFn open_adapter_fn_ = nullptr;
    wintunDeleteAdapterFn delete_adapter_fn_= nullptr;
    wintunStartSessionFn start_session_fn_= nullptr;
    wintunEndSessionFn end_session_fn_ = nullptr;
    wintunReceivedPacketFn received_packet_fn_ = nullptr;
    wintunSendPacketFn send_packet_fn_ = nullptr;

    friend struct WintunAdapter;
    friend struct WintunSession;
};
struct WintunAdapter {

    WintunAdapter(Wintun* wintun,WintunAdapter* handle) : wintun_(wintun), handle_(handle){}
    ~WintunAdapter();
    std::unique_ptr<WintunSession> start_session(size_t capacity);
    const wchar_t* name() const;

    Wintun* wintun_ = nullptr;
    WintunAdapter* handle_ = nullptr;
};

struct WintunSession {
    WintunSession(WintunAdapter* adapter, WintunSession* handle): adapter_(adapter), handle_(handle){}
    ~WintunSession();

    std::span<uint8_t> receive_packet(uint32_t timeout_ms, size_t* out_size);
    std::span<uint8_t> allocate_sd_packet(size_t size);
    void complete_send (size_t size);

    WintunAdapter* adapter_ = nullptr;
    WintunSession* handle_ = nullptr;
};

#endif //IVPN_WINTUN_H