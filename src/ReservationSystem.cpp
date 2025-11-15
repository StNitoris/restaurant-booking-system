#include "ReservationSystem.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace booking {

namespace {
constexpr int kDefaultSeatingDurationMinutes = 120;
}  // namespace

Permission::Permission(std::string name) : name_(std::move(name)) {}

const std::string &Permission::getName() const { return name_; }

Role::Role(std::string name, std::vector<Permission> permissions)
    : name_(std::move(name)), permissions_(std::move(permissions)) {}

const std::string &Role::getName() const { return name_; }

const std::vector<Permission> &Role::getPermissions() const { return permissions_; }

bool Role::hasPermission(const std::string &permission) const {
    return std::any_of(permissions_.begin(), permissions_.end(), [&](const Permission &p) {
        return p.getName() == permission;
    });
}

Staff::Staff(std::string name, std::string contact, Role role)
    : name_(std::move(name)), contact_(std::move(contact)), role_(std::move(role)) {}

const std::string &Staff::getName() const { return name_; }

const std::string &Staff::getContact() const { return contact_; }

const Role &Staff::getRole() const { return role_; }

FrontDeskStaff::FrontDeskStaff(std::string name, std::string contact)
    : Staff(std::move(name), std::move(contact), Role{"Front Desk", {Permission{"CreateReservation"}, Permission{"UpdateReservation"}}}) {}

Manager::Manager(std::string name, std::string contact)
    : Staff(std::move(name), std::move(contact),
            Role{"Manager", {Permission{"CreateReservation"},
                              Permission{"UpdateReservation"},
                              Permission{"ManageStaff"},
                              Permission{"ViewReports"}}}) {}

MenuItem::MenuItem(std::string name, std::string category, double price)
    : name_(std::move(name)), category_(std::move(category)), price_(price) {}

const std::string &MenuItem::getName() const { return name_; }

const std::string &MenuItem::getCategory() const { return category_; }

double MenuItem::getPrice() const { return price_; }

OrderItem::OrderItem(MenuItem item, int quantity) : item_(std::move(item)), quantity_(quantity) {}

const MenuItem &OrderItem::getItem() const { return item_; }

int OrderItem::getQuantity() const { return quantity_; }

double OrderItem::getLineTotal() const { return item_.getPrice() * static_cast<double>(quantity_); }

Order::Order(std::string id, std::string reservationId)
    : id_(std::move(id)), reservationId_(std::move(reservationId)) {}

const std::string &Order::getId() const { return id_; }

const std::string &Order::getReservationId() const { return reservationId_; }

void Order::addItem(const MenuItem &item, int quantity) {
    if (quantity <= 0) {
        return;
    }
    items_.emplace_back(item, quantity);
}

const std::vector<OrderItem> &Order::getItems() const { return items_; }

double Order::calculateTotal() const {
    double total = 0.0;
    for (const auto &item : items_) {
        total += item.getLineTotal();
    }
    return total;
}

Customer::Customer(std::string name, std::string phone, std::string email, std::string preference)
    : name_(std::move(name)),
      phone_(std::move(phone)),
      email_(std::move(email)),
      preference_(std::move(preference)) {}

const std::string &Customer::getName() const { return name_; }

const std::string &Customer::getPhone() const { return phone_; }

const std::string &Customer::getEmail() const { return email_; }

const std::string &Customer::getPreference() const { return preference_; }

Table::Table(int id, int capacity, std::string location)
    : id_(id), capacity_(capacity), location_(std::move(location)) {}

int Table::getId() const { return id_; }

int Table::getCapacity() const { return capacity_; }

TableStatus Table::getStatus() const { return status_; }

const std::string &Table::getLocation() const { return location_; }

void Table::setStatus(TableStatus status) { status_ = status; }

Reservation::Reservation(std::string id,
                         Customer customer,
                         int partySize,
                         std::chrono::system_clock::time_point time,
                         std::chrono::minutes duration,
                         std::string notes)
    : id_(std::move(id)),
      customer_(std::move(customer)),
      partySize_(partySize),
      time_(time),
      duration_(duration),
      notes_(std::move(notes)),
      lastModified_(std::chrono::system_clock::now()) {}

const std::string &Reservation::getId() const { return id_; }

const Customer &Reservation::getCustomer() const { return customer_; }

int Reservation::getPartySize() const { return partySize_; }

ReservationStatus Reservation::getStatus() const { return status_; }

std::chrono::system_clock::time_point Reservation::getDateTime() const { return time_; }

std::chrono::minutes Reservation::getDuration() const { return duration_; }

const std::string &Reservation::getNotes() const { return notes_; }

std::optional<int> Reservation::getTableId() const { return tableId_; }

std::chrono::system_clock::time_point Reservation::getLastModified() const { return lastModified_; }

void Reservation::assignTable(int tableId) {
    tableId_ = tableId;
    lastModified_ = std::chrono::system_clock::now();
}

void Reservation::clearTable() {
    tableId_.reset();
    lastModified_ = std::chrono::system_clock::now();
}

void Reservation::updateStatus(ReservationStatus status) {
    status_ = status;
    lastModified_ = std::chrono::system_clock::now();
}

void Reservation::setCustomer(Customer customer) {
    customer_ = std::move(customer);
    lastModified_ = std::chrono::system_clock::now();
}

void Reservation::setPartySize(int partySize) {
    partySize_ = partySize;
    lastModified_ = std::chrono::system_clock::now();
}

void Reservation::setDateTime(std::chrono::system_clock::time_point time) {
    time_ = time;
    lastModified_ = std::chrono::system_clock::now();
}

void Reservation::setDuration(std::chrono::minutes duration) {
    duration_ = duration;
    lastModified_ = std::chrono::system_clock::now();
}

void Reservation::setNotes(const std::string &notes) {
    notes_ = notes;
    lastModified_ = std::chrono::system_clock::now();
}

void Reservation::markSeated() { updateStatus(ReservationStatus::Seated); }

void Reservation::markCompleted() { updateStatus(ReservationStatus::Completed); }

void Reservation::cancel() { updateStatus(ReservationStatus::Cancelled); }

std::chrono::system_clock::time_point Reservation::getEndTime() const { return time_ + duration_; }

Report::Report(std::string date,
               int totalReservations,
               int seatedGuests,
               double revenue,
               std::vector<std::tuple<std::string, ReservationStatus>> reservationBreakdown)
    : date_(std::move(date)),
      totalReservations_(totalReservations),
      seatedGuests_(seatedGuests),
      revenue_(revenue),
      reservationBreakdown_(std::move(reservationBreakdown)) {}

const std::string &Report::getDate() const { return date_; }

int Report::getTotalReservations() const { return totalReservations_; }

int Report::getSeatedGuests() const { return seatedGuests_; }

double Report::getRevenue() const { return revenue_; }

const std::vector<std::tuple<std::string, ReservationStatus>> &Report::getReservationBreakdown() const {
    return reservationBreakdown_;
}

std::string Report::summary() const {
    std::ostringstream oss;
    oss << "Report for " << date_ << '\n';
    oss << "Total reservations: " << totalReservations_ << '\n';
    oss << "Guests seated: " << seatedGuests_ << '\n';
    oss << "Revenue: " << formatCurrency(revenue_) << '\n';
    oss << "Reservation breakdown:" << '\n';
    for (const auto &[id, status] : reservationBreakdown_) {
        oss << "  - " << id << ": ";
        switch (status) {
            case ReservationStatus::Open:
                oss << "Open";
                break;
            case ReservationStatus::Seated:
                oss << "Seated";
                break;
            case ReservationStatus::Completed:
                oss << "Completed";
                break;
            case ReservationStatus::Cancelled:
                oss << "Cancelled";
                break;
        }
        oss << '\n';
    }
    return oss.str();
}

BookingSheet::BookingSheet(std::string date) : date_(std::move(date)) {}

const std::string &BookingSheet::getDate() const { return date_; }

std::vector<Table> &BookingSheet::getTables() { return tables_; }

const std::vector<Table> &BookingSheet::getTables() const { return tables_; }

const std::vector<Reservation> &BookingSheet::getReservations() const { return reservations_; }

const std::vector<Order> &BookingSheet::getOrders() const { return orders_; }

void BookingSheet::addTable(const Table &table) { tables_.push_back(table); }

std::optional<int> BookingSheet::findAvailableTableId(int partySize,
                                                      std::chrono::system_clock::time_point time,
                                                      std::chrono::minutes duration,
                                                      const std::optional<std::string> &ignoreReservationId) const {
    auto ids = findAllAvailableTableIds(partySize, time, duration, ignoreReservationId);
    if (ids.empty()) {
        return std::nullopt;
    }
    return ids.front();
}

std::vector<int> BookingSheet::findAllAvailableTableIds(int partySize,
                                                        std::chrono::system_clock::time_point time,
                                                        std::chrono::minutes duration,
                                                        const std::optional<std::string> &ignoreReservationId) const {
    std::vector<int> ids;
    for (const auto &table : tables_) {
        if (table.getStatus() == TableStatus::OutOfService) {
            continue;
        }
        if (table.getCapacity() < partySize) {
            continue;
        }
        if (isTableAvailable(table.getId(), time, duration, ignoreReservationId)) {
            ids.push_back(table.getId());
        }
    }
    return ids;
}

Reservation &BookingSheet::createReservation(const Customer &customer,
                                             int partySize,
                                             std::chrono::system_clock::time_point time,
                                             std::chrono::minutes duration,
                                             const std::string &notes) {
    std::string id = "R" + std::to_string(nextReservationNumber_++);
    reservations_.emplace_back(id, customer, partySize, time, duration, notes);
    Reservation &reservation = reservations_.back();
    auto tableId = findAvailableTableId(partySize, time, duration);
    if (tableId) {
        reservation.assignTable(*tableId);
    }
    return reservation;
}

Reservation &BookingSheet::recordWalkIn(const Customer &customer, int partySize, const std::string &notes) {
    auto now = std::chrono::system_clock::now();
    auto &reservation = createReservation(customer, partySize, now,
                                          std::chrono::minutes(kDefaultSeatingDurationMinutes), notes);
    reservation.markSeated();
    return reservation;
}

Order &BookingSheet::recordOrder(const std::string &reservationId) {
    std::string id = "O" + std::to_string(nextOrderNumber_++);
    orders_.emplace_back(id, reservationId);
    return orders_.back();
}

Reservation *BookingSheet::findReservationById(const std::string &id) {
    auto it = std::find_if(reservations_.begin(), reservations_.end(), [&](const Reservation &reservation) {
        return reservation.getId() == id;
    });
    if (it == reservations_.end()) {
        return nullptr;
    }
    return &(*it);
}

const Reservation *BookingSheet::findReservationById(const std::string &id) const {
    auto it = std::find_if(reservations_.begin(), reservations_.end(), [&](const Reservation &reservation) {
        return reservation.getId() == id;
    });
    if (it == reservations_.end()) {
        return nullptr;
    }
    return &(*it);
}

Order *BookingSheet::findOrderById(const std::string &id) {
    auto it = std::find_if(orders_.begin(), orders_.end(), [&](const Order &order) { return order.getId() == id; });
    if (it == orders_.end()) {
        return nullptr;
    }
    return &(*it);
}

bool BookingSheet::updateReservationDetails(const std::string &id,
                                            const Customer &customer,
                                            int partySize,
                                            std::chrono::system_clock::time_point time,
                                            std::chrono::minutes duration,
                                            const std::string &notes,
                                            std::optional<int> requestedTable,
                                            bool tableSpecified) {
    auto reservation = findReservationById(id);
    if (!reservation) {
        return false;
    }

    std::optional<int> newTable;
    if (tableSpecified) {
        if (requestedTable) {
            const auto *table = getTableById(*requestedTable);
            if (!table || table->getCapacity() < partySize ||
                !isTableAvailable(*requestedTable, time, duration, reservation->getId())) {
                return false;
            }
            newTable = requestedTable;
        } else {
            newTable = std::nullopt;
        }
    } else {
        if (auto current = reservation->getTableId()) {
            const auto *table = getTableById(*current);
            if (table && table->getCapacity() >= partySize &&
                isTableAvailable(*current, time, duration, reservation->getId())) {
                newTable = *current;
            }
        }
        if (!newTable) {
            auto candidate = findAvailableTableId(partySize, time, duration, reservation->getId());
            if (candidate) {
                newTable = *candidate;
            }
        }
    }

    reservation->setCustomer(customer);
    reservation->setPartySize(partySize);
    reservation->setDateTime(time);
    reservation->setDuration(duration);
    reservation->setNotes(notes);

    if (newTable) {
        reservation->assignTable(*newTable);
    } else {
        reservation->clearTable();
    }

    return true;
}

bool BookingSheet::cancelReservation(const std::string &id) {
    auto reservation = findReservationById(id);
    if (!reservation) {
        return false;
    }
    reservation->cancel();
    reservation->clearTable();
    return true;
}

void BookingSheet::updateTableStatuses() {
    auto now = std::chrono::system_clock::now();
    for (auto &table : tables_) {
        if (table.getStatus() != TableStatus::OutOfService) {
            table.setStatus(TableStatus::Free);
        }
    }
    for (const auto &reservation : reservations_) {
        if (!reservation.getTableId() || reservation.getStatus() == ReservationStatus::Cancelled) {
            continue;
        }
        auto *table = getTableById(*reservation.getTableId());
        if (!table) {
            continue;
        }
        if (reservation.getStatus() == ReservationStatus::Completed) {
            continue;
        }
        auto start = reservation.getDateTime();
        auto end = reservation.getEndTime();
        if (reservation.getStatus() == ReservationStatus::Seated || (now >= start && now < end)) {
            table->setStatus(TableStatus::Occupied);
        } else if (now < start) {
            table->setStatus(TableStatus::Reserved);
        }
    }
}

void BookingSheet::updateDisplay(const std::function<void(const Reservation &)> &callback) const {
    for (const auto &reservation : reservations_) {
        callback(reservation);
    }
}

Report BookingSheet::generateReport() const {
    int seatedGuests = 0;
    std::vector<std::tuple<std::string, ReservationStatus>> breakdown;
    for (const auto &reservation : reservations_) {
        if (reservation.getStatus() == ReservationStatus::Seated ||
            reservation.getStatus() == ReservationStatus::Completed) {
            seatedGuests += reservation.getPartySize();
        }
        breakdown.emplace_back(reservation.getId(), reservation.getStatus());
    }

    double revenue = 0.0;
    for (const auto &order : orders_) {
        revenue += order.calculateTotal();
    }
    return Report(date_, static_cast<int>(reservations_.size()), seatedGuests, revenue, breakdown);
}

Table *BookingSheet::getTableById(int id) {
    auto it = std::find_if(tables_.begin(), tables_.end(), [&](const Table &table) { return table.getId() == id; });
    if (it == tables_.end()) {
        return nullptr;
    }
    return &(*it);
}

const Table *BookingSheet::getTableById(int id) const {
    auto it = std::find_if(tables_.begin(), tables_.end(), [&](const Table &table) { return table.getId() == id; });
    if (it == tables_.end()) {
        return nullptr;
    }
    return &(*it);
}

bool BookingSheet::isTableAvailable(int tableId,
                                     std::chrono::system_clock::time_point time,
                                     std::chrono::minutes duration,
                                     const std::optional<std::string> &ignoreReservationId) const {
    auto table = getTableById(tableId);
    if (!table || table->getStatus() == TableStatus::OutOfService) {
        return false;
    }
    auto newEnd = time + duration;
    for (const auto &reservation : reservations_) {
        if (reservation.getStatus() == ReservationStatus::Cancelled || !reservation.getTableId()) {
            continue;
        }
        if (ignoreReservationId && reservation.getId() == *ignoreReservationId) {
            continue;
        }
        if (*reservation.getTableId() != tableId) {
            continue;
        }
        auto existingStart = reservation.getDateTime();
        auto existingEnd = reservation.getEndTime();
        bool overlap = !(newEnd <= existingStart || time >= existingEnd);
        if (overlap) {
            return false;
        }
    }
    return true;
}

Restaurant::Restaurant(std::string name, std::string address, BookingSheet bookingSheet)
    : name_(std::move(name)), address_(std::move(address)), bookingSheet_(std::move(bookingSheet)) {}

const std::string &Restaurant::getName() const { return name_; }

const std::string &Restaurant::getAddress() const { return address_; }

BookingSheet &Restaurant::getBookingSheet() { return bookingSheet_; }

const BookingSheet &Restaurant::getBookingSheet() const { return bookingSheet_; }

void Restaurant::addMenuItem(const MenuItem &item) { menu_.push_back(item); }

const std::vector<MenuItem> &Restaurant::getMenu() const { return menu_; }

const MenuItem *Restaurant::findMenuItem(const std::string &name) const {
    auto it = std::find_if(menu_.begin(), menu_.end(), [&](const MenuItem &item) { return item.getName() == name; });
    if (it == menu_.end()) {
        return nullptr;
    }
    return &(*it);
}

void Restaurant::addStaff(std::shared_ptr<Staff> staff) { staff_.push_back(std::move(staff)); }

const std::vector<std::shared_ptr<Staff>> &Restaurant::getStaff() const { return staff_; }

Report Restaurant::generateDailyReport() const { return bookingSheet_.generateReport(); }

std::optional<std::chrono::system_clock::time_point> parseDateTime(const std::string &input) {
    std::tm tm = {};
    std::istringstream iss(input);
    iss >> std::get_time(&tm, "%Y-%m-%d %H:%M");
    if (iss.fail()) {
        return std::nullopt;
    }
    auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    return tp;
}

std::string formatDateTime(const std::chrono::system_clock::time_point &timePoint) {
    std::time_t t = std::chrono::system_clock::to_time_t(timePoint);
    std::tm tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M");
    return oss.str();
}

std::string formatCurrency(double value) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << "$" << value;
    return oss.str();
}

}  // namespace booking

