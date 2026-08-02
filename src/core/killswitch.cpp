#include "../../include/core/killswitch.h"
#include <spdlog/spdlog.h>
#include <fwpmu.h>

#include "tor/control_port.h"
const GUID FALLBACK_FWPM_CONDITION_ALE_APP_ID = {
    0xd78e1e87, 0x8644, 0x4ea5,
    { 0x94, 0x37, 0xd8, 0x09, 0xec, 0xef, 0xc9, 0x71 }
};
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
        filter.layerKey = FALLBACK_FWPM_LAYER_ALE_AUTH_CONNECT_V4;
        filter.subLayerKey = FALLBACK_FWPM_SUBLAYER_UNIVERSAL;
        filter.action.type = FWP_ACTION_BLOCK;
        filter.flags = 0;
        filter.weight.type = FWP_EMPTY;
        filter.numFilterConditions = 0;
        filter.filterCondition = nullptr;
        DWORD status = FwpmFilterAdd0(engine_, &filter, nullptr, &filter_id_);
        if (status == 0x80320023) {
            spdlog::info("WFP block filter already active. Continuing.");
        }
        else if (status != ERROR_SUCCESS) {
            spdlog::error("Failed to add block filter. Error Code: {}", status);
            return false;
        }

        active_ = true;
        
        FWPM_FILTER0 filter_v6{};
        filter_v6.layerKey = FALLBACK_FWPM_LAYER_ALE_AUTH_CONNECT_V6;
        filter_v6.subLayerKey = FALLBACK_FWPM_SUBLAYER_UNIVERSAL;
        filter_v6.action.type = FWP_ACTION_BLOCK;
        filter_v6.flags = 0;
        filter_v6.weight.type = FWP_EMPTY;
        filter_v6.numFilterConditions = 0;
        filter_v6.filterCondition = nullptr;
        
        DWORD status_v6 = FwpmFilterAdd0(engine_, &filter_v6, nullptr, &filter_id_v6_);
        if (status_v6 == 0x80320023) {
            spdlog::info("WFP IPv6 block filter already active. Continuing.");
        }
        else if (status_v6 != ERROR_SUCCESS) {
            spdlog::error("Failed to add IPv6 block filter. Error Code: {}", status_v6);
        }

        spdlog::info("Kill switch enabled (Blocking ALL outbound IPv4 and IPv6 traffic)");
        return true;
    }

    bool killSwitch::disable() {
        if (!engine_) return true;
        if (filter_id_ != 0) {
            DWORD status = FwpmFilterDeleteById0(engine_, filter_id_);
            if (status != ERROR_SUCCESS) {
                spdlog::error("failed to delete WFP IPv4 filter error Code: {}", status);
                return false;
            }
            filter_id_ = 0;
        }
        if (filter_id_v6_ != 0) {
            DWORD status = FwpmFilterDeleteById0(engine_, filter_id_v6_);
            if (status != ERROR_SUCCESS) {
                spdlog::error("Failed to delete WFP IPv6 filter. Error Code: {}", status);
            }
            filter_id_v6_ = 0;
        }
        close_session();
        active_ = false;
        spdlog::info("Kill switch disabled");
        return true;
    }

    bool killSwitch::is_active() const {
        return active_;
    }
    bool killSwitch::add_tor_permit_rule(const std::string &tor_exe_path) {
        if (!engine_) return false;
        std::wstring w_path(tor_exe_path.begin(), tor_exe_path.end());
        FWP_BYTE_BLOB* appId =nullptr;
        DWORD status = FwpmGetAppIdFromFileName0(w_path.c_str(),&appId);
        if (status != ERROR_SUCCESS) {
            spdlog::error("could not get App ID for tor. error: {}", status);
            return false;
        }
        FWPM_FILTER_CONDITION0 condition{};
        condition.fieldKey = FALLBACK_FWPM_CONDITION_ALE_APP_ID;
        condition.matchType = FWP_MATCH_EQUAL;
        condition.conditionValue.type = FWP_BYTE_BLOB_TYPE;
        condition.conditionValue.byteBlob = appId;
        FWPM_FILTER0 permitFilter{};
        permitFilter.layerKey = FALLBACK_FWPM_LAYER_ALE_AUTH_CONNECT_V4;
        permitFilter.subLayerKey = FALLBACK_FWPM_SUBLAYER_UNIVERSAL;
        permitFilter.action.type = FWP_ACTION_PERMIT;
        permitFilter.weight.type = FWP_UINT8;
        permitFilter.weight.uint8 = 15;
        permitFilter.numFilterConditions = 1;
        permitFilter.filterCondition = &condition;
        UINT64 permit_filter_id = 0;
        status = FwpmFilterAdd0(engine_, &permitFilter, nullptr,&permit_filter_id);
        FwpmFreeMemory0((void**)&appId);
        if (status == 0x80320023) {
            spdlog::info("WFP Tor permit rule already active. Continuing.");
        }
        else if (status != ERROR_SUCCESS) {
            spdlog::error("failed to add tor permit filter.error: {}", status);
            return false;
        }
        spdlog::info("Successfully added WFP permit rule for Tor");
        return true;

    }
}
