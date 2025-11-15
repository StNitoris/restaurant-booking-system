# Restaurant Reservation System

This project provides a simple C++ object-oriented implementation of a restaurant reservation system. It demonstrates the core behaviors from the provided use-case and analysis diagrams, including table allocation, reservation management, and reporting.

## Features

- Maintain restaurant tables and track their availability.
- Create, seat, complete, and cancel reservations with customer details and optional notes.
- Support preorder creation using menu and order abstractions.
- Manage menu items and staff roles with permission assignments.
- Generate a simple reservation activity report for the day.

## Building

The project uses CMake. To build the executable:

```bash
cmake -S . -B build
cmake --build build
```

The resulting binary is available at `build/restaurant_reservation`.

## Running

After building, run the demo application:

```bash
./build/restaurant_reservation
```

The console output walks through a sample flow of creating and managing reservations.

