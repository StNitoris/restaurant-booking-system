#pragma once

#include "ReservationSystem.hpp"

#include <string>

namespace booking {

void runWebServer(Restaurant &restaurant, const std::string &staticDir, int port = 8080);

}  // namespace booking

