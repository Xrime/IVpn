//
// Created by xint2 on 21/07/2026.
//
#include "../../include/UI/tui.h"

using namespace ftxui;
namespace ivpn::UI {
    tui::tui() : screen_(ftxui::ScreenInteractive::Fullscreen()) {
        std::vector<std::string> tab_entries = {"Dashboard", "Location"};
        auto tab_selection = Menu(&tab_entries, &tab_index_,MenuOption::HorizontalAnimated());
        auto btn_connect = Button("  CONNECT NOW  ",[&] {
           if (on_connect) on_connect();
        });
        auto btn_disconnecct = Button(" DISCONNECT ", [&] {
           if (on_disconnect) on_disconnect();
        });
        auto btn_quit = Button(" Quit ", [&] {
           if (on_quit) on_quit();
        });
        auto dashboard_container = Container::Horizontal({btn_connect, btn_disconnecct, btn_quit});
        auto dashboard_renderer= Renderer(dashboard_container, [&] {
           return vbox({
               text("IVpn") | bold | center,separator(),
               vbox({
                   text(is_connected_ ? "SECURE CONNECTION ACTIVE" : "CONNECTION OFFLINE")| color(is_connected_ ? Color::Green : Color::Red) | bold | center,text("Target Region: "+ current_city_) | center,
               }) | border, separator(),
               hbox({
                   is_connected_ ? btn_disconnecct->Render() : btn_connect->Render(), text("  "), btn_quit->Render()
               }) | center,
           });
        });
        auto city_menu = Menu(&cities_, &city_selected_);
        auto btn_apply_city = Button("Set Target Region", [&] {
           if (!cities_.empty() && city_selected_ >= 0 && city_selected_ < cities_.size()) {
               current_city_ = cities_[city_selected_];
               if (on_change_city) on_change_city(current_city_);
               tab_index_ = 0;
           }
        });
        auto location_container = Container::Vertical({
            city_menu,
            btn_apply_city
        });
        auto location_renderer = Renderer(location_container, [&] {
           return vbox({
               text("Select a routing destination:") | bold, separator(), city_menu->Render() | vscroll_indicator | frame | size(HEIGHT, LESS_THAN, 15), separator(), btn_apply_city->Render() | center
           });
        });

        auto tab_container = Container::Tab({
            dashboard_renderer,
            location_renderer
        }, &tab_index_);

        auto main_layout = Container::Vertical({
            tab_selection,
            tab_container
        });
        main_container = Renderer(main_layout,[&] {
           return vbox({
               tab_selection->Render() | center, separator(), tab_container->Render()
           }) | border;
        });
    }

    void tui::set_cities(const std::vector<std::string> &cities) {
        cities_ = cities;
        if (cities_.empty() || cities_[0] != "Auto") {
            cities_.insert(cities_.begin(), "Auto");
        }
    }
    void tui::set_current_city(const std::string &city) {
        current_city_ = city;
    }
    void tui::set_connected(bool connected) {
        is_connected_ = connected;
    }
    void tui::stop() {
        screen_.Exit();
    }
    void tui::redraw() {
        screen_.PostEvent(Event::Custom);
    }
    void tui::run() {
        screen_.Loop(main_container);
    }
}