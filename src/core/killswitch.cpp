//
// Created by xint2 on 21/07/2026.
//
#include "../../include/core/killswitch.h"
#include <spdlog/spdlog.h>


namespace ivpn::core {
    killSwitch::killSwitch() = default;
    killSwitch::~killSwitch() {
        disable();
    }
    bool killSwitch::openSession() {
        if (engine_) return true;
        FWPM_SESSION0 session{};
        session.flags = FWPM_SESSION_FLAG_DYNAMIC;
        HRESULT hr = FwpmEngineOpen0(
            nullptr,
   RPC_C_AUTHN_WINNT,
   nullptr,
            &session,
            &engine_
            );
        return SUCCEEDED(hr);
    }
    void killSwitch::close_session() {
        if (engine_) {
            FwpmEngineClose0(engine_);
            spdlog::error("Failed to open WFP engine");
        }
    }
    bool killSwitch::enable(uint16_t tor_socks_port) {
        if (!openSession()) {
            spdlog::error("Failed to open WFP engine");
            return false;
        }
        FWPM_FILTER0 filter{};
        filter.layerKey = FWPM_LAYER_OUTBOUND_TRANSPORT_V4;
        filter.action.type = FWP_ACTION_BLOCK;
        filter.flags = FWPM_FILTER_FLAG_PERSISTENT;
        filter.subLayerKey = FWPM_SUBLAYER_UNIVERSAL;
        filter.weight.type = FWP_EMPTY;
        FWPM_FILTER_CONDITION0 cond{};
        cond.fieldKey = FWPM_CONDITION_IP_REMOTE_PORT;
        cond.matchType = FWP_MATCH_EQUAL;
        cond.conditionValue.type = FWP_UINT16;
        cond.conditionValue.uint16 = tor_socks_port;
        filter.numFilterConditions = 1;
        filter.filterCondition = &cond;
        HRESULT hr = FwpmFilterAdd0(engine_, &filter, nullptr, &filter_id_);
        if (FAILED(hr)) {
            spdlog::error("Failed to add block filter: 0x{:X}", hr);
            return false;
        }

        active_ = true;
        spdlog::info("Kill switch enabled");
        return true;
    }

    bool killSwitch::disable() {
        if (!engine_ || filter_id_ == 0) return true;

        FwpmFilterDeleteByKey0(engine_, &filter_id_);
        close_session();
        active_ = false;
        return true;
    }

    bool killSwitch::is_active() const {
        return active_;
    }

}  // namespace ivpn::core