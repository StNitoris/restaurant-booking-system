#include "ReservationSystem.hpp"
#include "SeedData.hpp"
#include "WebServer.hpp"

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

using booking::BookingSheet;
using booking::Restaurant;

int main(int argc, char **argv) {
    int port = 8080;
    std::filesystem::path staticDir = "web";
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

    bool userProvidedStaticDir = argc > 2;
    auto existsDir = [](const std::filesystem::path &p) {
        return std::filesystem::exists(p) && std::filesystem::is_directory(p);
    };

    auto containsIndex = [](const std::filesystem::path &p) {
        return std::filesystem::exists(p / "index.html");
    };

    auto findStaticDir = [&](const std::filesystem::path &initial) -> std::optional<std::filesystem::path> {
        std::vector<std::filesystem::path> candidates;
        if (!initial.empty()) {
            candidates.push_back(initial);
        }
        std::filesystem::path exePath = std::filesystem::absolute(argv[0]).parent_path();
        candidates.push_back(exePath / "web");
        candidates.push_back(exePath.parent_path() / "web");
        candidates.push_back(std::filesystem::current_path() / "web");

        for (const auto &candidate : candidates) {
            if (existsDir(candidate) && containsIndex(candidate)) {
                return std::filesystem::absolute(candidate);
            }
        }
        return std::nullopt;
    };

    auto resolved = findStaticDir(userProvidedStaticDir ? staticDir : std::filesystem::path{});
    if (!resolved) {
        std::cerr << "Static directory not found (tried defaults near executable and current path)" << std::endl;
        return 1;
    }
    staticDir = *resolved;
    std::cout << "Serving static files from: " << staticDir << std::endl;

    Restaurant restaurant{"美味餐厅", "上海市黄浦区中山东一路12号", BookingSheet{"2024-05-20"}};
    booking::seedRestaurant(restaurant);

    try {
        booking::runWebServer(restaurant, staticDir.string(), port);
    } catch (const std::exception &ex) {
        std::cerr << "Failed to start web server: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}

