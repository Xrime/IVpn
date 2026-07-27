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

        DWORD status = FwpmEngineOpen0(
            nullptr,
            RPC_C_AUTHN_WINNT,
            nullptr,
            &session,
            &engine_
        );

        return (status == ERROR_SUCCESS);
    }

    void killSwitch::close_session() {
        if (engine_) {
            FwpmEngineClose0(engine_);
            engine_ = nullptr;
            spdlog::debug("WFP engine session closed successfully");
        }
    }

    bool killSwitch::enable() {
        if (!openSession()) {
            spdlog::error("Failed to open WFP engine");
            return false;
        }

        FWPM_FILTER0 filter{};

        // Use our fallback GUIDs to bypass MinGW compiler errors
        filter.layerKey = FALLBACK_FWPM_LAYER_OUTBOUND_TRANSPORT_V4;
        filter.subLayerKey = FALLBACK_FWPM_SUBLAYER_UNIVERSAL;

        filter.action.type = FWP_ACTION_BLOCK;
        filter.flags = FWPM_FILTER_FLAG_PERSISTENT;
        filter.weight.type = FWP_EMPTY;
        filter.numFilterConditions = 0;
        filter.filterCondition = nullptr;

        DWORD status = FwpmFilterAdd0(engine_, &filter, nullptr, &filter_id_);
        if (status != ERROR_SUCCESS) {
            spdlog::error("Failed to add block filter. Error Code: {}", status);
            return false;
        }

        active_ = true;
        spdlog::info("Kill switch enabled (Blocking ALL outbound IPv4 traffic)");
        return true;
    }

    bool killSwitch::disable() {
        if (!engine_ || filter_id_ == 0) return true;

        DWORD status = FwpmFilterDeleteById0(engine_, filter_id_);

        if (status != ERROR_SUCCESS) {
            spdlog::error("Failed to delete WFP filter. Error Code: {}", status);
            return false;
        }

        filter_id_ = 0;
        close_session();
        active_ = false;
        spdlog::info("Kill switch disabled");
        return true;
    }

    bool killSwitch::is_active() const {
        return active_;
    }

}