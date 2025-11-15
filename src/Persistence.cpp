#include "Persistence.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace booking {
namespace {

constexpr const char *kHeader = "BOOKING_DATA_V1";

std::string escapeField(const std::string &value) {
    std::string result;
    result.reserve(value.size());
    for (char ch : value) {
        switch (ch) {
            case '\\':
                result += "\\\\";
                break;
            case '|':
                result += "\\|";
                break;
            case '\n':
                result += "\\n";
                break;
            case '\r':
                result += "\\r";
                break;
            default:
                result.push_back(ch);
                break;
        }
    }
    return result;
}

std::string unescapeField(const std::string &value) {
    std::string result;
    result.reserve(value.size());
    bool escape = false;
    for (char ch : value) {
        if (escape) {
            switch (ch) {
                case '\\':
                    result.push_back('\\');
                    break;
                case 'n':
                    result.push_back('\n');
                    break;
                case 'r':
                    result.push_back('\r');
                    break;
                case '|':
                    result.push_back('|');
                    break;
                default:
                    result.push_back(ch);
                    break;
            }
            escape = false;
            continue;
        }
        if (ch == '\\') {
            escape = true;
            continue;
        }
        result.push_back(ch);
    }
    if (escape) {
        result.push_back('\\');
    }
    return result;
}

std::vector<std::string> splitEscaped(const std::string &line, char delimiter) {
    std::vector<std::string> parts;
    std::string current;
    current.reserve(line.size());
    bool escape = false;
    for (char ch : line) {
        if (escape) {
            if (ch == delimiter) {
                current.push_back(ch);
            } else {
                current.push_back('\\');
                current.push_back(ch);
            }
            escape = false;
            continue;
        }
        if (ch == '\\') {
            escape = true;
            continue;
        }
        if (ch == delimiter) {
            parts.push_back(current);
            current.clear();
            continue;
        }
        current.push_back(ch);
    }
    if (escape) {
        current.push_back('\\');
    }
    parts.push_back(current);
    return parts;
}

ReservationStatus reservationStatusFromInt(int value) {
    switch (value) {
        case 0:
            return ReservationStatus::Open;
        case 1:
            return ReservationStatus::Seated;
        case 2:
            return ReservationStatus::Completed;
        case 3:
            return ReservationStatus::Cancelled;
        default:
            return ReservationStatus::Open;
    }
}

TableStatus tableStatusFromInt(int value) {
    switch (value) {
        case 0:
            return TableStatus::Free;
        case 1:
            return TableStatus::Reserved;
        case 2:
            return TableStatus::Occupied;
        case 3:
            return TableStatus::OutOfService;
        default:
            return TableStatus::Free;
    }
}

int reservationStatusToInt(ReservationStatus status) {
    switch (status) {
        case ReservationStatus::Open:
            return 0;
        case ReservationStatus::Seated:
            return 1;
        case ReservationStatus::Completed:
            return 2;
        case ReservationStatus::Cancelled:
            return 3;
    }
    return 0;
}

int tableStatusToInt(TableStatus status) {
    switch (status) {
        case TableStatus::Free:
            return 0;
        case TableStatus::Reserved:
            return 1;
        case TableStatus::Occupied:
            return 2;
        case TableStatus::OutOfService:
            return 3;
    }
    return 0;
}

std::chrono::system_clock::time_point fromEpochSeconds(long long seconds) {
    return std::chrono::system_clock::time_point{std::chrono::seconds(seconds)};
}

long long toEpochSeconds(const std::chrono::system_clock::time_point &timePoint) {
    return std::chrono::duration_cast<std::chrono::seconds>(timePoint.time_since_epoch()).count();
}

bool ensureDirectory(const std::string &path, std::ostream *error) {
    try {
        std::filesystem::path target(path);
        auto parent = target.parent_path();
        if (!parent.empty() && !std::filesystem::exists(parent)) {
            std::filesystem::create_directories(parent);
        }
    } catch (const std::exception &ex) {
        if (error) {
            *error << "无法创建数据目录: " << ex.what() << '\n';
        }
        return false;
    }
    return true;
}

void bumpCounterFromId(const std::string &id, char prefix, int &counter) {
    if (id.size() <= 1 || id.front() != prefix) {
        return;
    }
    try {
        int value = std::stoi(id.substr(1));
        counter = std::max(counter, value + 1);
    } catch (...) {
    }
}

}  // namespace

bool loadBookingData(const std::string &path, Restaurant &restaurant, std::ostream *error) {
    std::ifstream file(path);
    if (!file) {
        return false;
    }

    std::string header;
    if (!std::getline(file, header) || header != kHeader) {
        if (error) {
            *error << "数据文件格式不受支持。" << '\n';
        }
        return false;
    }

    auto date = restaurant.getBookingSheet().getDate();
    int nextReservation = restaurant.getBookingSheet().getNextReservationNumber();
    int nextOrder = restaurant.getBookingSheet().getNextOrderNumber();

    std::vector<Table> tables;
    std::vector<Reservation> reservations;
    std::unordered_map<std::string, Order> orderMap;
    std::vector<std::string> orderSequence;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }
        auto parts = splitEscaped(line, '|');
        if (parts.empty()) {
            continue;
        }
        const auto &type = parts[0];
        try {
            if (type == "DATE") {
                if (parts.size() >= 2) {
                    date = unescapeField(parts[1]);
                }
            } else if (type == "NEXT_RESERVATION") {
                if (parts.size() >= 2) {
                    nextReservation = std::stoi(parts[1]);
                }
            } else if (type == "NEXT_ORDER") {
                if (parts.size() >= 2) {
                    nextOrder = std::stoi(parts[1]);
                }
            } else if (type == "TABLE") {
                if (parts.size() < 5) {
                    throw std::runtime_error("invalid table record");
                }
                int id = std::stoi(parts[1]);
                int capacity = std::stoi(parts[2]);
                std::string location = unescapeField(parts[3]);
                auto status = tableStatusFromInt(std::stoi(parts[4]));
                Table table{id, capacity, location};
                table.setStatus(status);
                tables.push_back(table);
            } else if (type == "RESERVATION") {
                if (parts.size() < 13) {
                    throw std::runtime_error("invalid reservation record");
                }
                std::string id = parts[1];
                Customer customer{unescapeField(parts[2]),
                                  unescapeField(parts[3]),
                                  unescapeField(parts[4]),
                                  unescapeField(parts[5])};
                int partySize = std::stoi(parts[6]);
                auto status = reservationStatusFromInt(std::stoi(parts[7]));
                long long startEpoch = std::stoll(parts[8]);
                int durationMinutes = std::stoi(parts[9]);
                int tableIdRaw = std::stoi(parts[10]);
                std::string notes = unescapeField(parts[11]);
                long long lastModified = std::stoll(parts[12]);

                Reservation reservation{id,
                                        customer,
                                        partySize,
                                        fromEpochSeconds(startEpoch),
                                        std::chrono::minutes(durationMinutes),
                                        notes};
                if (tableIdRaw >= 0) {
                    reservation.restoreTable(tableIdRaw);
                }
                reservation.restoreStatus(status);
                reservation.restoreLastModified(fromEpochSeconds(lastModified));
                reservations.push_back(reservation);
                bumpCounterFromId(id, 'R', nextReservation);
            } else if (type == "ORDER") {
                if (parts.size() < 3) {
                    throw std::runtime_error("invalid order record");
                }
                std::string id = parts[1];
                std::string reservationId = parts[2];
                Order order{id, reservationId};
                orderMap.emplace(id, order);
                orderSequence.push_back(id);
                bumpCounterFromId(id, 'O', nextOrder);
            } else if (type == "ORDER_ITEM") {
                if (parts.size() < 6) {
                    throw std::runtime_error("invalid order item record");
                }
                std::string orderId = parts[1];
                auto it = orderMap.find(orderId);
                if (it == orderMap.end()) {
                    continue;
                }
                MenuItem item{unescapeField(parts[2]), unescapeField(parts[3]), std::stod(parts[4])};
                int quantity = std::stoi(parts[5]);
                it->second.addItem(item, quantity);
            }
        } catch (const std::exception &ex) {
            if (error) {
                *error << "读取数据文件时出错: " << ex.what() << '\n';
            }
            return false;
        }
    }

    std::vector<Order> orders;
    orders.reserve(orderSequence.size());
    for (const auto &orderId : orderSequence) {
        auto it = orderMap.find(orderId);
        if (it != orderMap.end()) {
            orders.push_back(it->second);
        }
    }

    restaurant.getBookingSheet().replaceState(date, std::move(tables), std::move(reservations), std::move(orders),
                                              nextReservation, nextOrder);
    restaurant.getBookingSheet().updateTableStatuses();
    return true;
}

bool saveBookingData(const std::string &path, const Restaurant &restaurant, std::ostream *error) {
    if (!ensureDirectory(path, error)) {
        return false;
    }
    std::ofstream file(path, std::ios::trunc);
    if (!file) {
        if (error) {
            *error << "无法写入数据文件。" << '\n';
        }
        return false;
    }

    const auto &sheet = restaurant.getBookingSheet();
    file << kHeader << '\n';
    file << "DATE|" << escapeField(sheet.getDate()) << '\n';
    file << "NEXT_RESERVATION|" << sheet.getNextReservationNumber() << '\n';
    file << "NEXT_ORDER|" << sheet.getNextOrderNumber() << '\n';

    for (const auto &table : sheet.getTables()) {
        file << "TABLE|" << table.getId() << '|' << table.getCapacity() << '|' << escapeField(table.getLocation()) << '|' << tableStatusToInt(table.getStatus())
             << '\n';
    }

    for (const auto &reservation : sheet.getReservations()) {
        auto tableId = reservation.getTableId().value_or(-1);
        file << "RESERVATION|" << reservation.getId() << '|' << escapeField(reservation.getCustomer().getName()) << '|' << escapeField(reservation.getCustomer().getPhone())
             << '|' << escapeField(reservation.getCustomer().getEmail()) << '|' << escapeField(reservation.getCustomer().getPreference())
             << '|' << reservation.getPartySize() << '|' << reservationStatusToInt(reservation.getStatus()) << '|' << toEpochSeconds(reservation.getDateTime())
             << '|' << reservation.getDuration().count() << '|' << tableId << '|' << escapeField(reservation.getNotes()) << '|' << toEpochSeconds(reservation.getLastModified())
             << '\n';
    }

    for (const auto &order : sheet.getOrders()) {
        file << "ORDER|" << order.getId() << '|' << order.getReservationId() << '\n';
        for (const auto &item : order.getItems()) {
            file << "ORDER_ITEM|" << order.getId() << '|' << escapeField(item.getItem().getName()) << '|' << escapeField(item.getItem().getCategory())
                 << '|' << item.getItem().getPrice() << '|' << item.getQuantity() << '\n';
        }
    }

    return true;
}

}  // namespace booking
