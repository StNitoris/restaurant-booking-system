#include "SeedData.hpp"

namespace booking {

void seedRestaurant(Restaurant &restaurant) {
    auto &sheet = restaurant.getBookingSheet();
    sheet.addTable(Table{1, 2, "Window"});
    sheet.addTable(Table{2, 2, "Window"});
    sheet.addTable(Table{3, 4, "Center"});
    sheet.addTable(Table{4, 4, "Center"});
    sheet.addTable(Table{5, 6, "Patio"});

    restaurant.addMenuItem(MenuItem{"Seared Salmon", "Entree", 24.5});
    restaurant.addMenuItem(MenuItem{"Garden Salad", "Starter", 8.5});
    restaurant.addMenuItem(MenuItem{"Ribeye Steak", "Entree", 36.0});
    restaurant.addMenuItem(MenuItem{"Tiramisu", "Dessert", 7.5});
    restaurant.addMenuItem(MenuItem{"Fresh Lemonade", "Drink", 4.5});

    restaurant.addStaff(std::make_shared<FrontDeskStaff>("Alice", "alice@example.com"));
    restaurant.addStaff(std::make_shared<FrontDeskStaff>("Bob", "bob@example.com"));
    restaurant.addStaff(std::make_shared<Manager>("Grace", "grace@example.com"));
}

}  // namespace booking

