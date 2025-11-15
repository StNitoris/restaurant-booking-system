#include "reservation_system.hpp"

#include <algorithm>
#include <iomanip>
#include <stdexcept>

namespace restaurant {

namespace {
std::string toString(TableStatus status) {
    switch (status) {
    case TableStatus::Free:
        return "Free";
    case TableStatus::Reserved:
        return "Reserved";
    case TableStatus::Occupied:
        return "Occupied";
    case TableStatus::OutOfService:
        return "Out of service";
    }
    return "Unknown";
}

std::string toString(ReservationStatus status) {
    switch (status) {
    case ReservationStatus::Open:
        return "Open";
    case ReservationStatus::Booked:
        return "Booked";
    case ReservationStatus::Seated:
        return "Seated";
    case ReservationStatus::Completed:
        return "Completed";
    case ReservationStatus::Cancelled:
        return "Cancelled";
    }
    return "Unknown";
}

std::string toString(OrderStatus status) {
    switch (status) {
    case OrderStatus::Open:
        return "Open";
    case OrderStatus::Submitted:
        return "Submitted";
    case OrderStatus::Closed:
        return "Closed";
    case OrderStatus::Cancelled:
        return "Cancelled";
    }
    return "Unknown";
}

} // namespace

MenuItem::MenuItem(std::string name, std::string category, double price)
    : name_(std::move(name)), category_(std::move(category)), price_(price) {
    if (price < 0.0) {
        throw std::invalid_argument("Menu item price cannot be negative");
    }
}

const std::string &MenuItem::name() const noexcept { return name_; }

const std::string &MenuItem::category() const noexcept { return category_; }

double MenuItem::price() const noexcept { return price_; }

std::string MenuItem::display() const {
    std::ostringstream oss;
    oss << name_ << " (" << category_ << ") - $" << std::fixed << std::setprecision(2) << price_;
    return oss.str();
}

OrderItem::OrderItem(std::shared_ptr<MenuItem> item, std::size_t quantity)
    : item_(std::move(item)), quantity_(quantity) {
    if (!item_) {
        throw std::invalid_argument("Order item requires a valid menu item");
    }
    if (quantity_ == 0) {
        throw std::invalid_argument("Order item quantity must be greater than zero");
    }
}

const MenuItem &OrderItem::item() const noexcept { return *item_; }

std::size_t OrderItem::quantity() const noexcept { return quantity_; }

void OrderItem::setQuantity(std::size_t quantity) {
    if (quantity == 0) {
        throw std::invalid_argument("Quantity must be greater than zero");
    }
    quantity_ = quantity;
}

double OrderItem::subtotal() const noexcept { return item_->price() * static_cast<double>(quantity_); }

Order::Order(std::string id, OrderStatus status) : id_(std::move(id)), status_(status) {}

const std::string &Order::id() const noexcept { return id_; }

OrderStatus Order::status() const noexcept { return status_; }

void Order::updateStatus(OrderStatus status) { status_ = status; }

void Order::addItem(const std::shared_ptr<MenuItem> &item, std::size_t quantity) {
    if (!item) {
        throw std::invalid_argument("Cannot add a null menu item to an order");
    }
    auto it = std::find_if(items_.begin(), items_.end(), [&](const OrderItem &existing) {
        return existing.item().name() == item->name();
    });
    if (it != items_.end()) {
        it->setQuantity(it->quantity() + quantity);
    } else {
        items_.emplace_back(item, quantity);
    }
}

void Order::removeItem(const std::string &name) {
    items_.erase(std::remove_if(items_.begin(), items_.end(), [&](const OrderItem &existing) {
                        return existing.item().name() == name;
                    }),
                 items_.end());
}

const std::vector<OrderItem> &Order::items() const noexcept { return items_; }

double Order::total() const noexcept {
    double sum = 0.0;
    for (const auto &item : items_) {
        sum += item.subtotal();
    }
    return sum;
}

std::string Order::summary() const {
    std::ostringstream oss;
    oss << "Order " << id_ << " (" << toString(status_) << ")\n";
    for (const auto &item : items_) {
        oss << "  - " << item.item().name() << " x" << item.quantity() << " = $" << std::fixed << std::setprecision(2)
            << item.subtotal() << "\n";
    }
    oss << "Total: $" << std::fixed << std::setprecision(2) << total();
    return oss.str();
}

Table::Table(int number, int capacity)
    : number_(number), capacity_(capacity), status_(TableStatus::Free) {
    if (capacity <= 0) {
        throw std::invalid_argument("Table capacity must be positive");
    }
}

int Table::number() const noexcept { return number_; }

int Table::capacity() const noexcept { return capacity_; }

TableStatus Table::status() const noexcept { return status_; }

void Table::updateStatus(TableStatus status) { status_ = status; }

bool Table::isAvailable() const noexcept { return status_ == TableStatus::Free; }

std::string Table::display() const {
    std::ostringstream oss;
    oss << "Table " << number_ << " (capacity " << capacity_ << ") - " << toString(status_);
    return oss.str();
}

Customer::Customer(std::string name, std::string phone, std::string email)
    : name_(std::move(name)), phone_(std::move(phone)), email_(std::move(email)) {}

const std::string &Customer::name() const noexcept { return name_; }

const std::string &Customer::phone() const noexcept { return phone_; }

const std::string &Customer::email() const noexcept { return email_; }

std::string Customer::contactCard() const {
    std::ostringstream oss;
    oss << name_ << " | Phone: " << phone_;
    if (!email_.empty()) {
        oss << " | Email: " << email_;
    }
    return oss.str();
}

Permission::Permission(std::string name) : name_(std::move(name)) {}

const std::string &Permission::name() const noexcept { return name_; }

Role::Role(std::string roleName) : roleName_(std::move(roleName)) {}

const std::string &Role::name() const noexcept { return roleName_; }

void Role::addPermission(Permission permission) { permissions_.insert(permission.name()); }

bool Role::hasPermission(const std::string &permissionName) const noexcept {
    return permissions_.find(permissionName) != permissions_.end();
}

Staff::Staff(std::string name, Role role) : name_(std::move(name)), role_(std::move(role)) {}

const std::string &Staff::name() const noexcept { return name_; }

const Role &Staff::role() const noexcept { return role_; }

Reservation::Reservation(std::string id,
                         std::shared_ptr<Customer> customer,
                         int partySize,
                         std::string reservationTime)
    : id_(std::move(id)),
      customer_(std::move(customer)),
      partySize_(partySize),
      reservationTime_(std::move(reservationTime)),
      status_(ReservationStatus::Open) {
    if (!customer_) {
        throw std::invalid_argument("Reservation requires a valid customer");
    }
    if (partySize_ <= 0) {
        throw std::invalid_argument("Party size must be positive");
    }
}

const std::string &Reservation::id() const noexcept { return id_; }

const std::shared_ptr<Customer> &Reservation::customer() const noexcept { return customer_; }

int Reservation::partySize() const noexcept { return partySize_; }

const std::string &Reservation::reservationTime() const noexcept { return reservationTime_; }

ReservationStatus Reservation::status() const noexcept { return status_; }

std::shared_ptr<Table> Reservation::table() const noexcept { return table_; }

const std::vector<std::string> &Reservation::notes() const noexcept { return notes_; }

const std::optional<Order> &Reservation::preOrder() const noexcept { return preOrder_; }

void Reservation::assignTable(const std::shared_ptr<Table> &table) { table_ = table; }

void Reservation::addNote(std::string note) { notes_.push_back(std::move(note)); }

void Reservation::setStatus(ReservationStatus status) { status_ = status; }

void Reservation::setPreOrder(Order order) { preOrder_ = std::move(order); }

std::string Reservation::summary() const {
    std::ostringstream oss;
    oss << "Reservation " << id_ << " for " << customer_->name() << " at " << reservationTime_ << "\n";
    oss << "Party size: " << partySize_ << " | Status: " << toString(status_) << "\n";
    if (table_) {
        oss << "Table: " << table_->number() << " (capacity " << table_->capacity() << ")\n";
    }
    if (!notes_.empty()) {
        oss << "Notes: \n";
        for (const auto &note : notes_) {
            oss << "  - " << note << "\n";
        }
    }
    if (preOrder_) {
        oss << preOrder_->summary() << "\n";
    }
    return oss.str();
}

BookingSheet::BookingSheet() = default;

void BookingSheet::addTable(int number, int capacity) {
    if (findTable(number)) {
        throw std::invalid_argument("Table already exists");
    }
    tables_.push_back(std::make_shared<Table>(number, capacity));
}

std::shared_ptr<Table> BookingSheet::findTable(int number) const {
    auto it = std::find_if(tables_.begin(), tables_.end(), [&](const auto &table) {
        return table->number() == number;
    });
    if (it != tables_.end()) {
        return *it;
    }
    return nullptr;
}

std::vector<std::shared_ptr<Table>> BookingSheet::availableTables(int minCapacity) const {
    std::vector<std::shared_ptr<Table>> result;
    std::copy_if(tables_.begin(), tables_.end(), std::back_inserter(result), [&](const auto &table) {
        return table->isAvailable() && table->capacity() >= minCapacity;
    });
    return result;
}

std::shared_ptr<Reservation> BookingSheet::createReservation(const std::shared_ptr<Customer> &customer,
                                                             int partySize,
                                                             std::string reservationTime,
                                                             std::vector<std::string> notes) {
    auto table = allocateTable(partySize);
    if (!table) {
        throw std::runtime_error("No available table can accommodate the party size");
    }
    auto reservation = std::make_shared<Reservation>("R" + std::to_string(reservations_.size() + 1),
                                                     customer,
                                                     partySize,
                                                     std::move(reservationTime));
    for (auto &note : notes) {
        reservation->addNote(std::move(note));
    }
    reservation->assignTable(table);
    reservation->setStatus(ReservationStatus::Booked);
    table->updateStatus(TableStatus::Reserved);
    reservations_.push_back(reservation);
    return reservation;
}

void BookingSheet::cancelReservation(const std::string &reservationId) {
    auto reservation = findReservation(reservationId);
    if (!reservation) {
        throw std::runtime_error("Reservation not found");
    }
    reservation->setStatus(ReservationStatus::Cancelled);
    if (auto table = reservation->table()) {
        table->updateStatus(TableStatus::Free);
    }
}

void BookingSheet::seatReservation(const std::string &reservationId) {
    auto reservation = findReservation(reservationId);
    if (!reservation) {
        throw std::runtime_error("Reservation not found");
    }
    if (reservation->status() == ReservationStatus::Cancelled) {
        throw std::runtime_error("Cannot seat a cancelled reservation");
    }
    reservation->setStatus(ReservationStatus::Seated);
    if (auto table = reservation->table()) {
        table->updateStatus(TableStatus::Occupied);
    }
}

void BookingSheet::completeReservation(const std::string &reservationId) {
    auto reservation = findReservation(reservationId);
    if (!reservation) {
        throw std::runtime_error("Reservation not found");
    }
    reservation->setStatus(ReservationStatus::Completed);
    if (auto table = reservation->table()) {
        table->updateStatus(TableStatus::Free);
    }
}

std::vector<std::shared_ptr<Reservation>> BookingSheet::reservations() const { return reservations_; }

std::string BookingSheet::updateDisplay() const {
    std::ostringstream oss;
    oss << "=== Tables ===\n";
    for (const auto &table : tables_) {
        oss << table->display() << "\n";
    }
    oss << "\n=== Reservations ===\n";
    for (const auto &reservation : reservations_) {
        oss << reservation->summary() << "\n";
    }
    return oss.str();
}

std::shared_ptr<Reservation> BookingSheet::findReservation(const std::string &reservationId) const {
    auto it = std::find_if(reservations_.begin(), reservations_.end(), [&](const auto &reservation) {
        return reservation->id() == reservationId;
    });
    if (it != reservations_.end()) {
        return *it;
    }
    return nullptr;
}

std::shared_ptr<Table> BookingSheet::allocateTable(int partySize) const {
    auto candidates = availableTables(partySize);
    if (candidates.empty()) {
        return nullptr;
    }
    return *std::min_element(candidates.begin(), candidates.end(), [](const auto &a, const auto &b) {
        return a->capacity() < b->capacity();
    });
}

Report::Report(const BookingSheet &sheet) : sheet_(sheet) {}

std::string Report::buildDailySummary() const {
    std::ostringstream oss;
    const auto reservations = sheet_.reservations();
    int booked = 0;
    int seated = 0;
    int completed = 0;
    int cancelled = 0;
    for (const auto &reservation : reservations) {
        switch (reservation->status()) {
        case ReservationStatus::Booked:
            ++booked;
            break;
        case ReservationStatus::Seated:
            ++seated;
            break;
        case ReservationStatus::Completed:
            ++completed;
            break;
        case ReservationStatus::Cancelled:
            ++cancelled;
            break;
        case ReservationStatus::Open:
            break;
        }
    }
    oss << "Daily Reservation Summary\n";
    oss << "Booked: " << booked << "\n";
    oss << "Seated: " << seated << "\n";
    oss << "Completed: " << completed << "\n";
    oss << "Cancelled: " << cancelled << "\n";
    return oss.str();
}

Restaurant::Restaurant(std::string name, std::string address)
    : name_(std::move(name)), address_(std::move(address)), nextReservationId_(1) {}

void Restaurant::addTable(int number, int capacity) { bookingSheet_.addTable(number, capacity); }

void Restaurant::addMenuItem(std::string name, std::string category, double price) {
    menuItems_.push_back(std::make_shared<MenuItem>(std::move(name), std::move(category), price));
}

void Restaurant::hireStaff(std::shared_ptr<Staff> staffMember) { staff_.push_back(std::move(staffMember)); }

std::shared_ptr<Reservation> Restaurant::createReservation(const std::shared_ptr<Customer> &customer,
                                                           int partySize,
                                                           std::string reservationTime,
                                                           std::vector<std::string> notes) {
    auto reservation = bookingSheet_.createReservation(customer, partySize, std::move(reservationTime), std::move(notes));
    reservation->setStatus(ReservationStatus::Booked);
    reservation->assignTable(reservation->table());
    return reservation;
}

void Restaurant::cancelReservation(const std::string &reservationId) {
    bookingSheet_.cancelReservation(reservationId);
}

void Restaurant::seatReservation(const std::string &reservationId) {
    bookingSheet_.seatReservation(reservationId);
}

void Restaurant::completeReservation(const std::string &reservationId) {
    bookingSheet_.completeReservation(reservationId);
}

std::vector<std::shared_ptr<Table>> Restaurant::availableTables(int minCapacity) const {
    return bookingSheet_.availableTables(minCapacity);
}

std::vector<std::shared_ptr<Reservation>> Restaurant::reservations() const { return bookingSheet_.reservations(); }

std::vector<std::shared_ptr<MenuItem>> Restaurant::menu() const { return menuItems_; }

std::vector<std::shared_ptr<Staff>> Restaurant::staff() const { return staff_; }

Report Restaurant::buildReport() const { return Report{bookingSheet_}; }

std::string Restaurant::description() const {
    std::ostringstream oss;
    oss << name_ << " (" << address_ << ")";
    return oss.str();
}

} // namespace restaurant

