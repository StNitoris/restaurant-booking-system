#include "Persistence.hpp"
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
    std::string dataPath = "data/booking_data.txt";
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
    if (argc > 3) {
        dataPath = argv[3];
    }

    Restaurant restaurant{"美味餐厅", "上海市黄浦区中山东一路12号", BookingSheet{"2024-05-20"}};
    booking::seedRestaurant(restaurant);

    if (booking::loadBookingData(dataPath, restaurant, &std::cerr)) {
        std::cout << "已加载历史数据，共有 " << restaurant.getBookingSheet().getReservations().size()
                  << " 条预订记录。" << std::endl;
    } else {
        booking::saveBookingData(dataPath, restaurant, &std::cerr);
        std::cout << "首次运行，示例数据已写入 " << dataPath << std::endl;
    }

    auto persist = [&]() {
        if (!booking::saveBookingData(dataPath, restaurant, &std::cerr)) {
            std::cerr << "保存数据失败，请检查磁盘权限。" << std::endl;
        }
    };

    try {
        booking::runWebServer(restaurant, staticDir, port, persist);
    } catch (const std::exception &ex) {
        std::cerr << "Failed to start web server: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}

