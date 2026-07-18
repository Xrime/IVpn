//
// Created by xint2 on 18/07/2026.
//
#include "../../include/core/circuit_builder.h"
#include "../../include/tor/control_port.h"
#include <spdlog/spdlog.h>

ivpn::core::circuitBuilder::circuitBuilder(class controlPort &tor) : tor_(tor) {

}
bool ivpn::core::circuitBuilder::buildCircuit(uint32_t circuit_id, const std::vector<std::string> &exit_fingerprint) {
    std::string path;
    for (size_t i = 0; i < exit_fingerprint.size(); i++) {
        if (i > 0) {
            path += ",";
        }
        path += "$" + exit_fingerprint[i];
    }
    auto response = tor_.send(fmt::format("EXTENDCIRCUIT {} {}", circuit_id, path));
    if (response && response->find("250") ==0) {
        spdlog::info("Build circuit {} through {} nodes", circuit_id, exit_fingerprint.size());
        return true;
    }
    spdlog::error("Failed to build circuit: {}", response.value_or("no response"));
    return  false;
}
bool ivpn::core::circuitBuilder::closeCircuit(uint32_t circuit_id) {
    auto response = tor_.send(fmt::format("CLOSECIRCUIT {}", circuit_id));
    if (response && response->find("250") == 0) {
        spdlog::info("Closed circuit {}", circuit_id);
        return true;
    }
    return false;
}

