#include "reservation_system.hpp"

#include <iostream>

using namespace restaurant;

Role makeFrontDeskRole() {
    Role role{"Front Desk"};
    role.addPermission(Permission{"create_reservation"});
    role.addPermission(Permission{"cancel_reservation"});
    role.addPermission(Permission{"update_table_status"});
    return role;
}

Role makeManagerRole() {
    Role role{"Manager"};
    role.addPermission(Permission{"create_reservation"});
    role.addPermission(Permission{"cancel_reservation"});
    role.addPermission(Permission{"update_table_status"});
    role.addPermission(Permission{"manage_menu"});
    role.addPermission(Permission{"view_reports"});
    return role;
}

void displayMenu(const Restaurant &restaurant) {
    std::cout << "\n--- Menu ---\n";
    for (const auto &item : restaurant.menu()) {
        std::cout << "  " << item->display() << '\n';
    }
}

void displayReservations(const Restaurant &restaurant) {
    std::cout << "\n--- Reservations ---\n";
    for (const auto &reservation : restaurant.reservations()) {
        std::cout << reservation->summary() << '\n';
    }
}

int main() {
    Restaurant restaurant{"Ocean View", "123 Seaside Avenue"};

    // Setup tables
    restaurant.addTable(1, 2);
    restaurant.addTable(2, 4);
    restaurant.addTable(3, 6);

    // Setup menu
    restaurant.addMenuItem("Seared Salmon", "Main", 24.50);
    restaurant.addMenuItem("Garden Salad", "Starter", 9.75);
    restaurant.addMenuItem("Chocolate Lava Cake", "Dessert", 7.50);

    // Hire staff
    auto frontDesk = std::make_shared<Staff>("Avery Chen", makeFrontDeskRole());
    auto manager = std::make_shared<Staff>("Jordan Lee", makeManagerRole());
    restaurant.hireStaff(frontDesk);
    restaurant.hireStaff(manager);

    std::cout << "Welcome to " << restaurant.description() << "!" << '\n';
    displayMenu(restaurant);

    auto customerAlice = std::make_shared<Customer>("Alice Kim", "555-0101", "alice@example.com");
    auto customerNoah = std::make_shared<Customer>("Noah Patel", "555-0123");

    // Create reservations
    auto reservationAlice = restaurant.createReservation(customerAlice, 2, "2024-05-15 18:30", {"Window seat"});
    auto reservationNoah = restaurant.createReservation(customerNoah, 4, "2024-05-15 19:30", {"Birthday celebration"});

    // Add preorder for Alice
    Order preorder{"PO-1"};
    preorder.addItem(restaurant.menu().front(), 2);
    reservationAlice->setPreOrder(preorder);

    displayReservations(restaurant);

    // Update reservation statuses
    restaurant.seatReservation(reservationAlice->id());
    restaurant.completeReservation(reservationAlice->id());

    restaurant.seatReservation(reservationNoah->id());

    std::cout << "\nAfter seating updates:" << '\n';
    displayReservations(restaurant);

    // Cancel a reservation
    restaurant.cancelReservation(reservationNoah->id());
    std::cout << "\nAfter cancellation:" << '\n';
    displayReservations(restaurant);

    // Generate report
    auto report = restaurant.buildReport();
    std::cout << '\n' << report.buildDailySummary() << '\n';

    std::cout << "\nTable availability for party of 4:" << '\n';
    for (const auto &table : restaurant.availableTables(4)) {
        std::cout << "  " << table->display() << '\n';
    }

    return 0;
}

