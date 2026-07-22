//
// Created by xint2 on 21/07/2026.
//

#ifndef IVPN_TUI_H
#define IVPN_TUI_H
#include <string>
#include <vector>
#include <functional>

namespace ivpn::UI {
    class tui {
    public:
        enum class menu_option {
            connect,
            disconnect,
            change_city,
            list_cities,
            exit_app
        };
        tui() = default;
        void set_cities(const std::vector<std::string>& cities);
        menu_option show_menu(const std::string& current_city, int hops);
        void show_message(const std::string& msg);

    private:
        std::vector<std::string> cities_;
    };
}
#endif //IVPN_TUI_H