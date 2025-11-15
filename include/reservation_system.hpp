#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace restaurant {

enum class TableStatus {
    Free,
    Reserved,
    Occupied,
    OutOfService
};

enum class ReservationStatus {
    Open,
    Booked,
    Seated,
    Completed,
    Cancelled
};

enum class OrderStatus {
    Open,
    Submitted,
    Closed,
    Cancelled
};

class MenuItem {
  public:
    MenuItem(std::string name, std::string category, double price);

    const std::string &name() const noexcept;
    const std::string &category() const noexcept;
    double price() const noexcept;

    std::string display() const;

  private:
    std::string name_;
    std::string category_;
    double price_;
};

class OrderItem {
  public:
    OrderItem(std::shared_ptr<MenuItem> item, std::size_t quantity);

    const MenuItem &item() const noexcept;
    std::size_t quantity() const noexcept;
    void setQuantity(std::size_t quantity);
    double subtotal() const noexcept;

  private:
    std::shared_ptr<MenuItem> item_;
    std::size_t quantity_;
};

class Order {
  public:
    Order(std::string id, OrderStatus status = OrderStatus::Open);

    const std::string &id() const noexcept;
    OrderStatus status() const noexcept;
    void updateStatus(OrderStatus status);
    void addItem(const std::shared_ptr<MenuItem> &item, std::size_t quantity);
    void removeItem(const std::string &name);
    const std::vector<OrderItem> &items() const noexcept;
    double total() const noexcept;

    std::string summary() const;

  private:
    std::string id_;
    OrderStatus status_;
    std::vector<OrderItem> items_;
};

class Table {
  public:
    Table(int number, int capacity);

    int number() const noexcept;
    int capacity() const noexcept;
    TableStatus status() const noexcept;
    void updateStatus(TableStatus status);
    bool isAvailable() const noexcept;

    std::string display() const;

  private:
    int number_;
    int capacity_;
    TableStatus status_;
};

class Customer {
  public:
    Customer(std::string name, std::string phone, std::string email = {});

    const std::string &name() const noexcept;
    const std::string &phone() const noexcept;
    const std::string &email() const noexcept;

    std::string contactCard() const;

  private:
    std::string name_;
    std::string phone_;
    std::string email_;
};

class Permission {
  public:
    explicit Permission(std::string name);

    const std::string &name() const noexcept;

  private:
    std::string name_;
};

class Role {
  public:
    explicit Role(std::string roleName);

    const std::string &name() const noexcept;
    void addPermission(Permission permission);
    bool hasPermission(const std::string &permissionName) const noexcept;

  private:
    std::string roleName_;
    std::set<std::string> permissions_;
};

class Staff {
  public:
    Staff(std::string name, Role role);

    const std::string &name() const noexcept;
    const Role &role() const noexcept;

  private:
    std::string name_;
    Role role_;
};

class Reservation {
  public:
    Reservation(std::string id,
                std::shared_ptr<Customer> customer,
                int partySize,
                std::string reservationTime);

    const std::string &id() const noexcept;
    const std::shared_ptr<Customer> &customer() const noexcept;
    int partySize() const noexcept;
    const std::string &reservationTime() const noexcept;
    ReservationStatus status() const noexcept;
    std::shared_ptr<Table> table() const noexcept;
    const std::vector<std::string> &notes() const noexcept;
    const std::optional<Order> &preOrder() const noexcept;

    void assignTable(const std::shared_ptr<Table> &table);
    void addNote(std::string note);
    void setStatus(ReservationStatus status);
    void setPreOrder(Order order);

    std::string summary() const;

  private:
    std::string id_;
    std::shared_ptr<Customer> customer_;
    int partySize_;
    std::string reservationTime_;
    ReservationStatus status_;
    std::shared_ptr<Table> table_;
    std::vector<std::string> notes_;
    std::optional<Order> preOrder_;
};

class BookingSheet {
  public:
    BookingSheet();

    void addTable(int number, int capacity);
    std::shared_ptr<Table> findTable(int number) const;
    std::vector<std::shared_ptr<Table>> availableTables(int minCapacity) const;
    std::shared_ptr<Reservation> createReservation(const std::shared_ptr<Customer> &customer,
                                                   int partySize,
                                                   std::string reservationTime,
                                                   std::vector<std::string> notes = {});
    void cancelReservation(const std::string &reservationId);
    void seatReservation(const std::string &reservationId);
    void completeReservation(const std::string &reservationId);
    std::vector<std::shared_ptr<Reservation>> reservations() const;

    std::string updateDisplay() const;

  private:
    std::shared_ptr<Reservation> findReservation(const std::string &reservationId) const;
    std::shared_ptr<Table> allocateTable(int partySize) const;

    std::vector<std::shared_ptr<Table>> tables_;
    std::vector<std::shared_ptr<Reservation>> reservations_;
};

class Report {
  public:
    explicit Report(const BookingSheet &sheet);

    std::string buildDailySummary() const;

  private:
    const BookingSheet &sheet_;
};

class Restaurant {
  public:
    Restaurant(std::string name, std::string address);

    void addTable(int number, int capacity);
    void addMenuItem(std::string name, std::string category, double price);
    void hireStaff(std::shared_ptr<Staff> staffMember);

    std::shared_ptr<Reservation> createReservation(const std::shared_ptr<Customer> &customer,
                                                   int partySize,
                                                   std::string reservationTime,
                                                   std::vector<std::string> notes = {});
    void cancelReservation(const std::string &reservationId);
    void seatReservation(const std::string &reservationId);
    void completeReservation(const std::string &reservationId);

    std::vector<std::shared_ptr<Table>> availableTables(int minCapacity) const;
    std::vector<std::shared_ptr<Reservation>> reservations() const;
    std::vector<std::shared_ptr<MenuItem>> menu() const;
    std::vector<std::shared_ptr<Staff>> staff() const;

    Report buildReport() const;

    std::string description() const;

  private:
    std::string name_;
    std::string address_;
    BookingSheet bookingSheet_;
    std::vector<std::shared_ptr<MenuItem>> menuItems_;
    std::vector<std::shared_ptr<Staff>> staff_;
    mutable int nextReservationId_;
};

} // namespace restaurant

