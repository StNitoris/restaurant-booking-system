cmake_minimum_required(VERSION 3.16)
project(RestaurantReservationSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(restaurant_reservation
    src/main.cpp
    src/reservation_system.cpp)

target_include_directories(restaurant_reservation PRIVATE include)
