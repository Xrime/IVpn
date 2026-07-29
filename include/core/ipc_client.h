//
// Created by xint2 on 29/07/2026.
//

#ifndef IVPN_IPC_CLIENT_H
#define IVPN_IPC_CLIENT_H
#include <string>
namespace ivpn::core {
    class ipcClient {
    public:
        ipcClient(const std::string& pipe_name ="\\\\.\\pipe\\ivpn_control" );
        bool send_command(const std::string& command, const std::string& city = "");

    private:
        std::string pipe_name_;
    };
}
#endif //IVPN_IPC_CLIENT_H