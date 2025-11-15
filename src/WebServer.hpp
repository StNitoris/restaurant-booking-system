#pragma once

#include "ReservationSystem.hpp"

#include <functional>
#include <string>

namespace booking {

void runWebServer(Restaurant &restaurant,
                  const std::string &staticDir,
                  int port = 8080,
                  const std::function<void()> &onDataChanged = {});

}  // namespace booking

