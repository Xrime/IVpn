//
// Created by xint2 on 21/07/2026.
//

#ifndef IVPN_TUI_H
#define IVPN_TUI_H
#include <string>
#include <vector>
#include <functional>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

namespace ivpn::UI {
    class tui {
    public:
        tui();

        enum class menu_option {
            connect,
            disconnect,
            change_city,
            list_cities,
            exit_app
        };
        void set_cities(const std::vector<std::string>& cities);
        void set_current_city(const std::string& city);
        void set_connected(bool connected);
        void stop();
        std::function<void()> on_connect;
        std::function<void()> on_disconnect;
        std::function<void(const std::string&)> on_change_city;
        std::function<void()> on_quit;
        void redraw();
        void run();

    private:
        ftxui::ScreenInteractive screen_;
        std::vector<std::string> cities_;
        std::string current_city_ = "Auto";
        bool is_connected_ =  false;
        int tab_index_ =0;
        int city_selected_ =0;
        ftxui::Component main_container;
    };
}
#endif //IVPN_TUI_H