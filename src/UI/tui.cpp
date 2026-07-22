//
// Created by xint2 on 21/07/2026.
//
#include "../../include/UI/tui.h"
#include <iostream>
#include <limits>

namespace ivpn::UI {
    void tui::set_cities(const std::vector<std::string> &cities) {
        cities_ = cities;
    }
    tui::menu_option tui::show_menu(const std::string &current_city, int hops) {
        while (true) {
            system("cls");
            std::cout << "=== IVpn Terminal Menu ===\n";
            std::cout <<"Current: City=" << current_city << ", Hops= "<< hops << "\n\n";
            std::cout << "1. Connect \n";
            std::cout <<"2. Disconnection";
            std::cout << "3. Change City";
            std::cout<< "4. List Avaliable Cities\n";
            std::cout <<"5. Exit\n";
            std::cout << "Select: ";

            int choice;
            std::cin >> choice;
            switch (choice) {
                case 1: return menu_option::connect;
                case 2: return menu_option::disconnect;
                case 3: return menu_option::change_city;
                case 5: return menu_option::exit_app;
                case 4: return menu_option::list_cities;

            }
            
        }
    }
    void tui::show_message(const std::string &msg) {
        std::cout << "INFO" <<msg <<"\n";
    }



}