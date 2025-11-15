#pragma once

#include "ReservationSystem.hpp"

#include <iosfwd>
#include <string>

namespace booking {

bool loadBookingData(const std::string &path, Restaurant &restaurant, std::ostream *error = nullptr);
bool saveBookingData(const std::string &path, const Restaurant &restaurant, std::ostream *error = nullptr);

}  // namespace booking
