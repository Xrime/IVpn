//
// Created by xint2 on 18/07/2026.
//

#ifndef IVPN_CIRCUIT_BUILDER_H
#define IVPN_CIRCUIT_BUILDER_H

#include <cstdint>
#include <string>
#include <vector>

#include "exit_selector.h"

using namespace ivpn::tor;
namespace ivpn::core {
    class circuitBuilder {
    public:
        circuitBuilder(class controlPort& tor);
        bool buildCircuit(uint32_t circuit_id, const std::vector<std::string>& exit_fingerprint);
        bool closeCircuit(uint32_t circuit_id);

    private:
        controlPort& tor_;
    };

}

#endif //IVPN_CIRCUIT_BUILDER_H