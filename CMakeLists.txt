cmake_minimum_required(VERSION 3.16)
project(restaurant_booking LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BOOKING_SOURCES
    src/ReservationSystem.cpp
    src/SeedData.cpp
)

add_library(booking_core STATIC ${BOOKING_SOURCES})
target_include_directories(booking_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(restaurant_booking
    src/main.cpp
)
target_link_libraries(restaurant_booking PRIVATE booking_core)

add_executable(restaurant_booking_server
    src/web_main.cpp
    src/WebServer.cpp
)

if (WIN32)
    target_link_libraries(restaurant_booking_server PRIVATE booking_core ws2_32)
else()
    target_link_libraries(restaurant_booking_server PRIVATE booking_core pthread)
endif()

