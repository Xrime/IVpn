#include "../../include/UI/tui.h"

using namespace ftxui;

namespace ivpn::UI {

    tui::tui() : screen_(ftxui::ScreenInteractive::Fullscreen()) {
        auto tab_selection = Menu(&tab_entries_, &tab_index_, MenuOption::HorizontalAnimated());
        ButtonOption toggle_option = ButtonOption::Animated();
        toggle_option.transform = [this](const EntryState& s) {
            auto bg = s.focused ? (is_connected_ ? bgcolor(Color::DarkRed) : bgcolor(Color::DarkGreen))
                                : bgcolor(Color::Default);
            auto fg = is_connected_ ? color(Color::RedLight) : color(Color::GreenLight);
            if (s.focused) fg = color(Color::White);
            std::string text_label = is_connected_ ? "DISCONNECT" : " CONNECT  ";
            return vbox({
                text("     ╭────────╮     ") | center,
                text("   ╭─╯        ╰─╮   ") | center,
                text("  ╭╯            ╰╮  ") | center,
                text("  │  " + text_label + "  │  ") | center,
                text("  ╰╮            ╭╯  ") | center,
                text("   ╰─╮        ╭─╯   ") | center,
                text("     ╰────────╯     ") | center
            }) | center | bg | fg | bold;
        };
        auto btn_toggle = Button("Toggle", [this] {
            if (is_connected_) {
                if (on_disconnect) on_disconnect();
            } else {
                if (on_connect) on_connect();
            }
        }, toggle_option);
        auto btn_quit = Button(" Quit IVpn ", [this] {
           if (on_quit) on_quit();
        });
        auto dashboard_container = Container::Vertical({btn_toggle, btn_quit});
        auto dashboard_renderer = Renderer(dashboard_container, [this, btn_toggle, btn_quit] {
           return vbox({
               text("IVpn") | bold | center | color(Color::Cyan),
               text("Target Region: " + current_city_) | center | dim,
               separatorEmpty() | size(HEIGHT, EQUAL, 3),
               btn_toggle->Render() | center,
               separatorEmpty() | size(HEIGHT, EQUAL, 2),text(is_connected_ ? "STATUS: SECURE & ENCRYPTED" : "STATUS: UNPROTECTED")
                   | bold | center | color(is_connected_ ? Color::Green : Color::Red),filler(),btn_quit->Render() | center
           }) | flex;
        });
        auto city_menu = Menu(&cities_, &city_selected_);
        ButtonOption apply_option = ButtonOption::Animated();
        apply_option.transform = [](const EntryState& s) {
            auto bg = s.focused ? bgcolor(Color::BlueLight) : bgcolor(Color::GrayDark);
            auto fg = s.focused ? color(Color::White) : color(Color::White);
            return text("   SET TARGET REGION   ") | bold | center | bg | fg;
        };

        auto btn_apply_city = Button("Apply", [this] {
           if (!cities_.empty() && city_selected_ >= 0 && city_selected_ < cities_.size()) {
               current_city_ = cities_[city_selected_];
               if (on_change_city) on_change_city(current_city_);
               tab_index_ = 0;
           }
        }, apply_option);
        auto location_container = Container::Vertical({
            city_menu,
            btn_apply_city
        });
        auto location_renderer = Renderer(location_container, [city_menu, btn_apply_city] {
           return vbox({
               text("Select a Routing Destination") | bold | center,
               separator(),
               city_menu->Render() | vscroll_indicator | frame | size(HEIGHT, LESS_THAN, 20),
               separator(),
               btn_apply_city->Render() | center
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
        main_container = Renderer(main_layout, [tab_selection, tab_container] {
           return vbox({
               tab_selection->Render() | center,
               separator(),
               tab_container->Render() | flex
           }) | border;
        });
    }
    void tui::set_cities(const std::vector<std::string> &cities) {
        cities_ = cities;
        if (cities_.empty() || cities_[0] != "Auto (Random)") {
            cities_.insert(cities_.begin(), "Auto (Random)");
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