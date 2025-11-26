#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace booking {

enum class TableStatus {
    Free,
    Reserved,
    Occupied,
    OutOfService
};

enum class ReservationStatus {
    Open,
    Seated,
    Completed,
    Cancelled
};

class Permission {
public:
    explicit Permission(std::string name);
    const std::string &getName() const;

private:
    std::string name_;
};

class Role {
public:
    Role(std::string name, std::vector<Permission> permissions = {});

    const std::string &getName() const;
    const std::vector<Permission> &getPermissions() const;
    bool hasPermission(const std::string &permission) const;

private:
    std::string name_;
    std::vector<Permission> permissions_;
};

class Staff {
public:
    Staff(std::string name, std::string contact, Role role);
    virtual ~Staff() = default;

    const std::string &getName() const;
    const std::string &getContact() const;
    const Role &getRole() const;

private:
    std::string name_;
    std::string contact_;
    Role role_;
};

class FrontDeskStaff : public Staff {
public:
    FrontDeskStaff(std::string name, std::string contact);
};

class Manager : public Staff {
public:
    Manager(std::string name, std::string contact);
};

class MenuItem {
public:
    MenuItem(std::string name, std::string category, double price);

    const std::string &getName() const;
    const std::string &getCategory() const;
    double getPrice() const;

private:
    std::string name_;
    std::string category_;
    double price_{};
};

class OrderItem {
public:
    OrderItem(MenuItem item, int quantity);

    const MenuItem &getItem() const;
    int getQuantity() const;
    double getLineTotal() const;

private:
    MenuItem item_;
    int quantity_{};
};

class Order {
public:
    Order(std::string id, std::string reservationId);

    const std::string &getId() const;
    const std::string &getReservationId() const;
    void addItem(const MenuItem &item, int quantity);
    const std::vector<OrderItem> &getItems() const;
    double calculateTotal() const;

private:
    std::string id_;
    std::string reservationId_;
    std::vector<OrderItem> items_;
};

class Customer {
public:
    Customer(std::string name, std::string phone, std::string email = {}, std::string preference = {});

    const std::string &getName() const;
    const std::string &getPhone() const;
    const std::string &getEmail() const;
    const std::string &getPreference() const;

private:
    std::string name_;
    std::string phone_;
    std::string email_;
    std::string preference_;
};

class Table {
public:
    Table(int id, int capacity, std::string location = {});

    int getId() const;
    int getCapacity() const;
    TableStatus getStatus() const;
    const std::string &getLocation() const;
    void setStatus(TableStatus status);

private:
    int id_;
    int capacity_;
    std::string location_;
    TableStatus status_ = TableStatus::Free;
};

class Reservation {
public:
    Reservation(std::string id,
                Customer customer,
                int partySize,
                std::chrono::system_clock::time_point time,
                std::chrono::minutes duration,
                std::string notes = {});

    const std::string &getId() const;
    const Customer &getCustomer() const;
    int getPartySize() const;
    ReservationStatus getStatus() const;
    std::chrono::system_clock::time_point getDateTime() const;
    std::chrono::minutes getDuration() const;
    const std::string &getNotes() const;
    std::optional<int> getTableId() const;
    std::chrono::system_clock::time_point getLastModified() const;

    void assignTable(int tableId);
    void clearTable();
    void updateStatus(ReservationStatus status);
    void setCustomer(Customer customer);
    void setPartySize(int partySize);
    void setDateTime(std::chrono::system_clock::time_point time);
    void setDuration(std::chrono::minutes duration);
    void setNotes(const std::string &notes);
    void markSeated();
    void markCompleted();
    void cancel();
    std::chrono::system_clock::time_point getEndTime() const;

private:
    std::string id_;
    Customer customer_;
    int partySize_;
    std::chrono::system_clock::time_point time_;
    std::chrono::minutes duration_;
    ReservationStatus status_ = ReservationStatus::Open;
    std::optional<int> tableId_;
    std::string notes_;
    std::chrono::system_clock::time_point lastModified_;
};

class Report {
public:
    Report(std::string date,
           int totalReservations,
           int seatedGuests,
           double revenue,
           std::vector<std::tuple<std::string, ReservationStatus>> reservationBreakdown);

    const std::string &getDate() const;
    int getTotalReservations() const;
    int getSeatedGuests() const;
    double getRevenue() const;
    const std::vector<std::tuple<std::string, ReservationStatus>> &getReservationBreakdown() const;
    std::string summary() const;

private:
    std::string date_;
    int totalReservations_{};
    int seatedGuests_{};
    double revenue_{};
    std::vector<std::tuple<std::string, ReservationStatus>> reservationBreakdown_;
};

class BookingSheet {
public:
    explicit BookingSheet(std::string date);

    const std::string &getDate() const;
    std::vector<Table> &getTables();
    const std::vector<Table> &getTables() const;
    const std::vector<Reservation> &getReservations() const;
    const std::vector<Order> &getOrders() const;

    void addTable(const Table &table);
    std::optional<int> findAvailableTableId(int partySize,
                                            std::chrono::system_clock::time_point time,
                                            std::chrono::minutes duration,
                                            const std::optional<std::string> &ignoreReservationId = std::nullopt) const;
    std::vector<int> findAllAvailableTableIds(int partySize,
                                              std::chrono::system_clock::time_point time,
                                              std::chrono::minutes duration,
                                              const std::optional<std::string> &ignoreReservationId = std::nullopt) const;

    Reservation &createReservation(const Customer &customer,
                                   int partySize,
                                   std::chrono::system_clock::time_point time,
                                   std::chrono::minutes duration,
                                   const std::string &notes = {});
    Reservation &recordWalkIn(const Customer &customer, int partySize, const std::string &notes = {});
    bool autoAssignTable(const std::string &id);
    bool assignTable(const std::string &id, int tableId);
    bool clearTableAssignment(const std::string &id);
    Order &recordOrder(const std::string &reservationId);
    Reservation *findReservationById(const std::string &id);
    const Reservation *findReservationById(const std::string &id) const;
    Order *findOrderById(const std::string &id);
    bool deleteReservation(const std::string &id);
    bool updateReservationDetails(const std::string &id,
                                  const Customer &customer,
                                  int partySize,
                                  std::chrono::system_clock::time_point time,
                                  std::chrono::minutes duration,
                                  const std::string &notes,
                                  std::optional<int> requestedTable,
                                  bool tableSpecified);
    bool cancelReservation(const std::string &id);
    void updateTableStatuses();
    void updateDisplay(const std::function<void(const Reservation &)> &callback) const;
    Report generateReport() const;

private:
    Table *getTableById(int id);
    const Table *getTableById(int id) const;
    bool isTableAvailable(int tableId,
                          std::chrono::system_clock::time_point time,
                          std::chrono::minutes duration,
                          const std::optional<std::string> &ignoreReservationId = std::nullopt) const;
    Reservation &createReservationRecord(std::string id,
                                         const Customer &customer,
                                         int partySize,
                                         std::chrono::system_clock::time_point time,
                                         std::chrono::minutes duration,
                                         const std::string &notes);
    std::string formatNumberedId(char prefix, int number) const;

    std::string date_;
    std::vector<Table> tables_;
    std::vector<Reservation> reservations_;
    std::vector<Order> orders_;
    int nextReservationNumber_ = 1000;
    int nextWalkInNumber_ = 5000;
    int nextOrderNumber_ = 1;
};

class Restaurant {
public:
    Restaurant(std::string name, std::string address, BookingSheet bookingSheet);

    const std::string &getName() const;
    const std::string &getAddress() const;
    BookingSheet &getBookingSheet();
    const BookingSheet &getBookingSheet() const;

    void addMenuItem(const MenuItem &item);
    const std::vector<MenuItem> &getMenu() const;
    const MenuItem *findMenuItem(const std::string &name) const;

    void addStaff(std::shared_ptr<Staff> staff);
    const std::vector<std::shared_ptr<Staff>> &getStaff() const;

    Report generateDailyReport() const;

private:
    std::string name_;
    std::string address_;
    BookingSheet bookingSheet_;
    std::vector<MenuItem> menu_;
    std::vector<std::shared_ptr<Staff>> staff_;
};

std::optional<std::chrono::system_clock::time_point> parseDateTime(const std::string &input);
std::string formatDateTime(const std::chrono::system_clock::time_point &timePoint);
std::string formatCurrency(double value);

}  // namespace booking

