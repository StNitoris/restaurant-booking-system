#include "ReservationSystem.hpp"
#include "SeedData.hpp"
#include "WebServer.hpp"

#include <iostream>
#include <string>

using booking::BookingSheet;
using booking::Restaurant;

int main(int argc, char **argv) {
    int port = 8080;
    std::string staticDir = "web";
    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "Invalid port, fallback to 8080" << std::endl;
            port = 8080;
        }
    }
    if (argc > 2) {
        staticDir = argv[2];
    }

    Restaurant restaurant{"美味餐厅", "上海市黄浦区中山东一路12号", BookingSheet{"2024-05-20"}};
    booking::seedRestaurant(restaurant);

    try {
        booking::runWebServer(restaurant, staticDir, port);
    } catch (const std::exception &ex) {
        std::cerr << "Failed to start web server: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}

