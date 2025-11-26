#include "WebServer.hpp"

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <chrono>
#include <cerrno>
#include <cctype>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace booking {
namespace {

#ifdef _WIN32
using SocketHandle = SOCKET;
constexpr SocketHandle INVALID_SOCKET_HANDLE = INVALID_SOCKET;

void closeSocket(SocketHandle socket) { closesocket(socket); }

class SocketEnvironment {
public:
    SocketEnvironment() {
        WSADATA data;
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
            throw std::runtime_error("Failed to initialize Winsock");
        }
    }

    SocketEnvironment(const SocketEnvironment &) = delete;
    SocketEnvironment &operator=(const SocketEnvironment &) = delete;

    ~SocketEnvironment() { WSACleanup(); }
};
#else
using SocketHandle = int;
constexpr SocketHandle INVALID_SOCKET_HANDLE = -1;

void closeSocket(SocketHandle socket) { close(socket); }

class SocketEnvironment {
public:
    SocketEnvironment() = default;
    SocketEnvironment(const SocketEnvironment &) = delete;
    SocketEnvironment &operator=(const SocketEnvironment &) = delete;
};
#endif

#ifdef _WIN32
using SendSize = int;
#else
using SendSize = ssize_t;
#endif

struct HttpRequest {
    std::string method;
    std::string path;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

struct HttpResponse {
    int status = 200;
    std::string contentType = "application/json; charset=utf-8";
    std::string body;
    std::vector<std::pair<std::string, std::string>> headers;
};

std::string statusMessage(int status) {
    switch (status) {
        case 200:
            return "OK";
        case 201:
            return "Created";
        case 204:
            return "No Content";
        case 400:
            return "Bad Request";
        case 404:
            return "Not Found";
        case 500:
        default:
            return "Internal Server Error";
    }
}

bool iequals(char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); }

bool headerEquals(const std::string &header, const std::string &key) {
    if (header.size() != key.size()) {
        return false;
    }
    for (size_t i = 0; i < header.size(); ++i) {
        if (!iequals(header[i], key[i])) {
            return false;
        }
    }
    return true;
}

bool startsWith(const std::string &value, const std::string &prefix) {
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

bool endsWith(const std::string &value, const std::string &suffix) {
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::optional<int> toInt(const std::string &value) {
    try {
        return std::stoi(value);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<std::string> getHeader(const HttpRequest &req, const std::string &key) {
    auto it = req.headers.find(key);
    if (it != req.headers.end()) {
        return it->second;
    }
    // try case-insensitive match
    for (const auto &entry : req.headers) {
        if (headerEquals(entry.first, key)) {
            return entry.second;
        }
    }
    return std::nullopt;
}

bool parseRequestLine(const std::string &line, HttpRequest &request) {
    std::istringstream iss(line);
    if (!(iss >> request.method >> request.path)) {
        return false;
    }
    return true;
}

std::string trim(const std::string &value) {
    auto start = value.find_first_not_of(" \t");
    auto end = value.find_last_not_of(" \t");
    if (start == std::string::npos || end == std::string::npos) {
        return "";
    }
    return value.substr(start, end - start + 1);
}

bool parseHeaders(std::istringstream &stream, HttpRequest &request) {
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            break;
        }
        auto colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }
        std::string key = line.substr(0, colonPos);
        std::string value = trim(line.substr(colonPos + 1));
        request.headers[key] = value;
    }
    return true;
}

bool readHttpRequest(SocketHandle clientFd, HttpRequest &request) {
    std::string buffer;
    buffer.reserve(4096);
    char temp[4096];
    int received = 0;

    while (buffer.find("\r\n\r\n") == std::string::npos) {
        received = recv(clientFd, temp, sizeof(temp), 0);
        if (received <= 0) {
            return false;
        }
        buffer.append(temp, static_cast<size_t>(received));
        if (buffer.size() > 1'000'000) {
            return false;
        }
    }

    auto headerEnd = buffer.find("\r\n\r\n");
    std::string headerPart = buffer.substr(0, headerEnd);
    std::string bodyPart = buffer.substr(headerEnd + 4);

    std::istringstream headerStream(headerPart);
    std::string requestLine;
    if (!std::getline(headerStream, requestLine)) {
        return false;
    }
    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back();
    }
    if (!parseRequestLine(requestLine, request)) {
        return false;
    }
    parseHeaders(headerStream, request);

    size_t contentLength = 0;
    if (auto header = getHeader(request, "Content-Length")) {
        try {
            contentLength = static_cast<size_t>(std::stoul(*header));
        } catch (...) {
            contentLength = 0;
        }
    }

    while (bodyPart.size() < contentLength) {
        received = recv(clientFd, temp, sizeof(temp), 0);
        if (received <= 0) {
            return false;
        }
        bodyPart.append(temp, static_cast<size_t>(received));
        if (bodyPart.size() > 1'000'000) {
            return false;
        }
    }

    request.body = bodyPart.substr(0, contentLength);
    return true;
}

std::string urlDecode(const std::string &value) {
    std::string result;
    result.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '+') {
            result.push_back(' ');
        } else if (value[i] == '%' && i + 2 < value.size()) {
            std::string hex = value.substr(i + 1, 2);
            char decoded = static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
            result.push_back(decoded);
            i += 2;
        } else {
            result.push_back(value[i]);
        }
    }
    return result;
}

using FormValues = std::unordered_map<std::string, std::vector<std::string>>;

FormValues parseFormEncoded(const std::string &body) {
    FormValues data;
    size_t start = 0;
    while (start < body.size()) {
        auto end = body.find('&', start);
        if (end == std::string::npos) {
            end = body.size();
        }
        auto pair = body.substr(start, end - start);
        auto eqPos = pair.find('=');
        if (eqPos != std::string::npos) {
            auto key = urlDecode(pair.substr(0, eqPos));
            auto value = urlDecode(pair.substr(eqPos + 1));
            data[key].push_back(value);
        }
        start = end + 1;
    }
    return data;
}

bool hasField(const FormValues &data, const std::string &key) {
    auto it = data.find(key);
    return it != data.end() && !it->second.empty();
}

std::optional<std::string> getFirstField(const FormValues &data, const std::string &key) {
    auto it = data.find(key);
    if (it == data.end() || it->second.empty()) {
        return std::nullopt;
    }
    return it->second.front();
}

std::vector<std::string> getAllFields(const FormValues &data, const std::string &key) {
    auto it = data.find(key);
    if (it == data.end()) {
        return {};
    }
    return it->second;
}

std::string escapeJson(const std::string &value) {
    std::ostringstream oss;
    for (char ch : value) {
        switch (ch) {
            case '\\':
                oss << "\\\\";
                break;
            case '"':
                oss << "\\\"";
                break;
            case '\n':
                oss << "\\n";
                break;
            case '\r':
                oss << "\\r";
                break;
            case '\t':
                oss << "\\t";
                break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20) {
                    oss << "\\u" << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
                        << static_cast<int>(static_cast<unsigned char>(ch)) << std::nouppercase << std::dec;
                } else {
                    oss << ch;
                }
                break;
        }
    }
    return oss.str();
}

std::string tableStatusToString(TableStatus status) {
    switch (status) {
        case TableStatus::Free:
            return "Free";
        case TableStatus::Reserved:
            return "Reserved";
        case TableStatus::Occupied:
            return "Occupied";
        case TableStatus::OutOfService:
            return "OutOfService";
    }
    return "Unknown";
}

std::string reservationStatusToString(ReservationStatus status) {
    switch (status) {
        case ReservationStatus::Open:
            return "Open";
        case ReservationStatus::Seated:
            return "Seated";
        case ReservationStatus::Completed:
            return "Completed";
        case ReservationStatus::Cancelled:
            return "Cancelled";
    }
    return "Unknown";
}

std::optional<ReservationStatus> parseReservationStatus(const std::string &value) {
    if (value == "Open") {
        return ReservationStatus::Open;
    }
    if (value == "Seated") {
        return ReservationStatus::Seated;
    }
    if (value == "Completed") {
        return ReservationStatus::Completed;
    }
    if (value == "Cancelled") {
        return ReservationStatus::Cancelled;
    }
    return std::nullopt;
}

std::string tablesToJson(const Restaurant &restaurant) {
    std::ostringstream oss;
    oss << '[';
    const auto &sheet = restaurant.getBookingSheet();
    const auto &tables = sheet.getTables();
    const auto &reservations = sheet.getReservations();
    const auto &orders = sheet.getOrders();
    for (size_t i = 0; i < tables.size(); ++i) {
        const auto &table = tables[i];
        if (i > 0) {
            oss << ',';
        }
        oss << '{'
            << "\"id\":" << table.getId() << ','
            << "\"capacity\":" << table.getCapacity() << ','
            << "\"location\":\"" << escapeJson(table.getLocation()) << "\",";
        oss << "\"status\":\"" << tableStatusToString(table.getStatus()) << "\",";
        oss << "\"reservations\":[";
        bool firstReservation = true;
        for (const auto &reservation : reservations) {
            if (!reservation.getTableId() || *reservation.getTableId() != table.getId()) {
                continue;
            }
            if (reservation.getStatus() == ReservationStatus::Cancelled) {
                continue;
            }
            if (!firstReservation) {
                oss << ',';
            }
            firstReservation = false;
            oss << '{'
                << "\"id\":\"" << escapeJson(reservation.getId()) << "\",";
            oss << "\"customer\":\"" << escapeJson(reservation.getCustomer().getName()) << "\",";
            oss << "\"partySize\":" << reservation.getPartySize() << ',';
            oss << "\"status\":\"" << reservationStatusToString(reservation.getStatus()) << "\",";
            oss << "\"orders\":[";
            bool firstOrder = true;
            for (const auto &order : orders) {
                if (order.getReservationId() != reservation.getId()) {
                    continue;
                }
                if (!firstOrder) {
                    oss << ',';
                }
                firstOrder = false;
                oss << "\"" << escapeJson(order.getId()) << "\"";
            }
            oss << ']';
            oss << '}';
        }
        oss << ']';
        oss << '}';
    }
    oss << ']';
    return oss.str();
}

std::string reservationToJson(const Reservation &reservation) {
    std::ostringstream oss;
    oss << '{';
    oss << "\"id\":\"" << escapeJson(reservation.getId()) << "\",";
    oss << "\"customer\":\"" << escapeJson(reservation.getCustomer().getName()) << "\",";
    oss << "\"phone\":\"" << escapeJson(reservation.getCustomer().getPhone()) << "\",";
    oss << "\"email\":\"" << escapeJson(reservation.getCustomer().getEmail()) << "\",";
    oss << "\"preference\":\"" << escapeJson(reservation.getCustomer().getPreference()) << "\",";
    oss << "\"partySize\":" << reservation.getPartySize() << ',';
    oss << "\"time\":\"" << escapeJson(formatDateTime(reservation.getDateTime())) << "\",";
    oss << "\"endTime\":\"" << escapeJson(formatDateTime(reservation.getEndTime())) << "\",";
    oss << "\"durationMinutes\":" << reservation.getDuration().count() << ',';
    oss << "\"status\":\"" << reservationStatusToString(reservation.getStatus()) << "\",";
    oss << "\"notes\":\"" << escapeJson(reservation.getNotes()) << "\",";
    oss << "\"tableId\":";
    if (reservation.getTableId()) {
        oss << *reservation.getTableId();
    } else {
        oss << "null";
    }
    oss << ',';
    oss << "\"lastModified\":\"" << escapeJson(formatDateTime(reservation.getLastModified())) << "\"";
    oss << '}';
    return oss.str();
}

std::string reservationsToJson(const Restaurant &restaurant) {
    std::ostringstream oss;
    oss << '[';
    const auto &reservations = restaurant.getBookingSheet().getReservations();
    for (size_t i = 0; i < reservations.size(); ++i) {
        if (i > 0) {
            oss << ',';
        }
        oss << reservationToJson(reservations[i]);
    }
    oss << ']';
    return oss.str();
}

std::string ordersToJson(const Restaurant &restaurant) {
    std::ostringstream oss;
    oss << '[';
    const auto &orders = restaurant.getBookingSheet().getOrders();
    for (size_t i = 0; i < orders.size(); ++i) {
        const auto &order = orders[i];
        if (i > 0) {
            oss << ',';
        }
        oss << '{';
        oss << "\"id\":\"" << escapeJson(order.getId()) << "\",";
        oss << "\"reservationId\":\"" << escapeJson(order.getReservationId()) << "\",";
        oss << "\"total\":" << order.calculateTotal() << ',';
        oss << "\"items\":[";
        const auto &items = order.getItems();
        for (size_t j = 0; j < items.size(); ++j) {
            const auto &item = items[j];
            if (j > 0) {
                oss << ',';
            }
            oss << '{';
            oss << "\"name\":\"" << escapeJson(item.getItem().getName()) << "\",";
            oss << "\"category\":\"" << escapeJson(item.getItem().getCategory()) << "\",";
            oss << "\"price\":" << item.getItem().getPrice() << ',';
            oss << "\"quantity\":" << item.getQuantity() << ',';
            oss << "\"lineTotal\":" << item.getLineTotal();
            oss << '}';
        }
        oss << ']';
        oss << '}';
    }
    oss << ']';
    return oss.str();
}

std::string menuToJson(const Restaurant &restaurant) {
    std::ostringstream oss;
    oss << '[';
    const auto &menu = restaurant.getMenu();
    for (size_t i = 0; i < menu.size(); ++i) {
        const auto &item = menu[i];
        if (i > 0) {
            oss << ',';
        }
        oss << '{';
        oss << "\"name\":\"" << escapeJson(item.getName()) << "\",";
        oss << "\"category\":\"" << escapeJson(item.getCategory()) << "\",";
        oss << "\"price\":" << item.getPrice();
        oss << '}';
    }
    oss << ']';
    return oss.str();
}

std::string staffToJson(const Restaurant &restaurant) {
    std::ostringstream oss;
    oss << '[';
    const auto &staff = restaurant.getStaff();
    for (size_t i = 0; i < staff.size(); ++i) {
        const auto &member = staff[i];
        if (i > 0) {
            oss << ',';
        }
        oss << '{';
        oss << "\"name\":\"" << escapeJson(member->getName()) << "\",";
        oss << "\"role\":\"" << escapeJson(member->getRole().getName()) << "\",";
        oss << "\"contact\":\"" << escapeJson(member->getContact()) << "\"";
        oss << '}';
    }
    oss << ']';
    return oss.str();
}

std::string reportToJson(const Report &report) {
    std::ostringstream oss;
    oss << '{';
    oss << "\"date\":\"" << escapeJson(report.getDate()) << "\",";
    oss << "\"totalReservations\":" << report.getTotalReservations() << ',';
    oss << "\"seatedGuests\":" << report.getSeatedGuests() << ',';
    oss << "\"revenue\":" << report.getRevenue() << ',';
    oss << "\"breakdown\":[";
    const auto &breakdown = report.getReservationBreakdown();
    for (size_t i = 0; i < breakdown.size(); ++i) {
        if (i > 0) {
            oss << ',';
        }
        oss << '{';
        oss << "\"reservationId\":\"" << escapeJson(std::get<0>(breakdown[i])) << "\",";
        oss << "\"status\":\"" << reservationStatusToString(std::get<1>(breakdown[i])) << "\"";
        oss << '}';
    }
    oss << "]}";
    return oss.str();
}

bool hasHeader(const HttpResponse &response, const std::string &key) {
    for (const auto &header : response.headers) {
        if (headerEquals(header.first, key)) {
            return true;
        }
    }
    return false;
}

void ensureHeader(HttpResponse &response, const std::string &key, const std::string &value) {
    if (!hasHeader(response, key)) {
        response.headers.emplace_back(key, value);
    }
}

void applyCorsHeaders(HttpResponse &response, bool includeMethods) {
    ensureHeader(response, "Access-Control-Allow-Origin", "*");
    if (includeMethods) {
        ensureHeader(response, "Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        ensureHeader(response, "Access-Control-Allow-Headers", "Content-Type");
        ensureHeader(response, "Access-Control-Max-Age", "86400");
    }
}

std::string buildResponse(const HttpResponse &response) {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << response.status << ' ' << statusMessage(response.status) << "\r\n";
    oss << "Content-Type: " << response.contentType << "\r\n";
    oss << "Content-Length: " << response.body.size() << "\r\n";
    for (const auto &header : response.headers) {
        oss << header.first << ": " << header.second << "\r\n";
    }
    oss << "Connection: close\r\n\r\n";
    oss << response.body;
    return oss.str();
}

std::string guessMimeType(const std::string &path) {
    if (endsWith(path, ".html")) {
        return "text/html; charset=utf-8";
    }
    if (endsWith(path, ".css")) {
        return "text/css; charset=utf-8";
    }
    if (endsWith(path, ".js")) {
        return "application/javascript; charset=utf-8";
    }
    if (endsWith(path, ".json")) {
        return "application/json; charset=utf-8";
    }
    if (endsWith(path, ".png")) {
        return "image/png";
    }
    if (endsWith(path, ".jpg") || endsWith(path, ".jpeg")) {
        return "image/jpeg";
    }
    return "text/plain; charset=utf-8";
}

bool isSafePath(const std::string &path) {
    return path.find("..") == std::string::npos;
}

std::optional<std::string> readFile(const std::string &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

HttpResponse serveStaticFile(const std::string &root, const std::string &path) {
    std::string resolvedPath = path;
    if (resolvedPath == "/") {
        resolvedPath = "/index.html";
    }
    if (!isSafePath(resolvedPath)) {
        return {404, "text/plain; charset=utf-8", "Not Found"};
    }
    std::string fullPath = root + resolvedPath;
    auto content = readFile(fullPath);
    if (!content) {
        return {404, "text/plain; charset=utf-8", "Not Found"};
    }
    HttpResponse response;
    response.status = 200;
    response.contentType = guessMimeType(fullPath);
    response.body = *content;
    return response;
}

HttpResponse buildPreflightResponse() {
    HttpResponse response;
    response.status = 204;
    response.contentType = "text/plain; charset=utf-8";
    response.body.clear();
    applyCorsHeaders(response, true);
    return response;
}

HttpResponse handleApiRequest(const HttpRequest &request,
                              Restaurant &restaurant,
                              std::mutex &mutex) {
    HttpResponse response;
    std::lock_guard<std::mutex> lock(mutex);

    auto &sheet = restaurant.getBookingSheet();
    sheet.updateTableStatuses();

    if (request.method == "GET" && request.path == "/api/tables") {
        response.body = tablesToJson(restaurant);
        return response;
    }

    if (request.method == "GET" && request.path == "/api/reservations") {
        response.body = reservationsToJson(restaurant);
        return response;
    }

    const std::string reservationIdPrefix = "/api/reservations/";
    auto isReservationIdPath = [&](const std::string &path) {
        return startsWith(path, reservationIdPrefix) &&
               path.size() > reservationIdPrefix.size() &&
               path.find('/', reservationIdPrefix.size()) == std::string::npos;
    };

    if (request.method == "GET" && isReservationIdPath(request.path)) {
        auto id = request.path.substr(reservationIdPrefix.size());
        auto reservation = sheet.findReservationById(id);
        if (!reservation) {
            return {404, "text/plain; charset=utf-8", "Reservation not found"};
        }
        response.body = reservationToJson(*reservation);
        return response;
    }

    if (request.method == "GET" && request.path == "/api/orders") {
        response.body = ordersToJson(restaurant);
        return response;
    }

    if (request.method == "GET" && request.path == "/api/menu") {
        response.body = menuToJson(restaurant);
        return response;
    }

    if (request.method == "GET" && request.path == "/api/staff") {
        response.body = staffToJson(restaurant);
        return response;
    }

    if (request.method == "GET" && request.path == "/api/report") {
        auto report = restaurant.generateDailyReport();
        response.body = reportToJson(report);
        return response;
    }

    if (request.method == "POST" && request.path == "/api/reservations") {
        auto data = parseFormEncoded(request.body);
        if (!hasField(data, "name") || !hasField(data, "phone") || !hasField(data, "partySize") ||
            !hasField(data, "time")) {
            return {400, "text/plain; charset=utf-8", "Missing required fields"};
        }
        auto partySizeOpt = toInt(*getFirstField(data, "partySize"));
        if (!partySizeOpt || *partySizeOpt <= 0) {
            return {400, "text/plain; charset=utf-8", "Invalid party size"};
        }
        auto timePoint = parseDateTime(*getFirstField(data, "time"));
        if (!timePoint) {
            return {400, "text/plain; charset=utf-8", "Invalid time format"};
        }
        Customer customer{*getFirstField(data, "name"),
                          *getFirstField(data, "phone"),
                          getFirstField(data, "email").value_or(""),
                          getFirstField(data, "preference").value_or("")};
        auto &reservation = sheet.createReservation(customer,
                                                    *partySizeOpt,
                                                    *timePoint,
                                                    std::chrono::minutes(120),
                                                    getFirstField(data, "notes").value_or(""));
        response.status = 201;
        response.body = "{\"success\":true,\"id\":\"" + escapeJson(reservation.getId()) + "\"}";
        return response;
    }

    if (request.method == "POST" && request.path == "/api/walkins") {
        auto data = parseFormEncoded(request.body);
        if (!hasField(data, "name") || !hasField(data, "phone") || !hasField(data, "partySize")) {
            return {400, "text/plain; charset=utf-8", "Missing required fields"};
        }
        auto partySizeOpt = toInt(*getFirstField(data, "partySize"));
        if (!partySizeOpt || *partySizeOpt <= 0) {
            return {400, "text/plain; charset=utf-8", "Invalid party size"};
        }
        Customer customer{*getFirstField(data, "name"), *getFirstField(data, "phone")};
        auto &reservation = sheet.recordWalkIn(customer, *partySizeOpt, getFirstField(data, "notes").value_or(""));
        response.status = 201;
        response.body = "{\"success\":true,\"id\":\"" + escapeJson(reservation.getId()) + "\"}";
        return response;
    }

    if (request.method == "POST" && request.path == "/api/orders") {
        auto data = parseFormEncoded(request.body);
        if (!hasField(data, "reservationId")) {
            return {400, "text/plain; charset=utf-8", "Missing reservationId"};
        }
        auto reservationId = *getFirstField(data, "reservationId");
        auto reservation = sheet.findReservationById(reservationId);
        if (!reservation) {
            return {404, "text/plain; charset=utf-8", "Reservation not found"};
        }
        auto rawItems = getAllFields(data, "items");
        if (rawItems.empty()) {
            return {400, "text/plain; charset=utf-8", "No items supplied"};
        }

        std::vector<std::pair<const MenuItem *, int>> parsedItems;
        parsedItems.reserve(rawItems.size());
        for (const auto &entry : rawItems) {
            auto delimiter = entry.find('|');
            if (delimiter == std::string::npos) {
                return {400, "text/plain; charset=utf-8", "Invalid item format"};
            }
            auto name = entry.substr(0, delimiter);
            auto quantityOpt = toInt(entry.substr(delimiter + 1));
            if (!quantityOpt || *quantityOpt <= 0) {
                return {400, "text/plain; charset=utf-8", "Invalid quantity"};
            }
            const auto *menuItem = restaurant.findMenuItem(name);
            if (!menuItem) {
                return {400, "text/plain; charset=utf-8", "Unknown menu item"};
            }
            parsedItems.emplace_back(menuItem, *quantityOpt);
        }

        auto &order = sheet.recordOrder(reservationId);
        for (const auto &[item, quantity] : parsedItems) {
            order.addItem(*item, quantity);
        }
        response.status = 201;
        std::ostringstream body;
        body << "{\"success\":true,\"id\":\"" << escapeJson(order.getId()) << "\",\"total\":"
             << order.calculateTotal() << "}";
        response.body = body.str();
        return response;
    }

    if ((request.method == "PUT" || request.method == "DELETE") && isReservationIdPath(request.path)) {
        auto id = request.path.substr(reservationIdPrefix.size());
        if (request.method == "DELETE") {
            if (!sheet.cancelReservation(id)) {
                return {404, "text/plain; charset=utf-8", "Reservation not found"};
            }
            response.body = "{\"success\":true}";
            return response;
        }

        auto reservation = sheet.findReservationById(id);
        if (!reservation) {
            return {404, "text/plain; charset=utf-8", "Reservation not found"};
        }

        auto data = parseFormEncoded(request.body);
        if (!hasField(data, "name") || !hasField(data, "phone") || !hasField(data, "partySize") ||
            !hasField(data, "time")) {
            return {400, "text/plain; charset=utf-8", "Missing required fields"};
        }

        auto partySizeOpt = toInt(*getFirstField(data, "partySize"));
        if (!partySizeOpt || *partySizeOpt <= 0) {
            return {400, "text/plain; charset=utf-8", "Invalid party size"};
        }

        auto durationMinutes = toInt(getFirstField(data, "durationMinutes").value_or("120"));
        if (!durationMinutes || *durationMinutes <= 0) {
            return {400, "text/plain; charset=utf-8", "Invalid duration"};
        }

        auto timePoint = parseDateTime(*getFirstField(data, "time"));
        if (!timePoint) {
            return {400, "text/plain; charset=utf-8", "Invalid time format"};
        }

        bool tableSpecified = hasField(data, "tableId");
        std::optional<int> requestedTable;
        if (tableSpecified) {
            auto tableField = getFirstField(data, "tableId");
            if (tableField && !tableField->empty()) {
                auto parsed = toInt(*tableField);
                if (!parsed || *parsed <= 0) {
                    return {400, "text/plain; charset=utf-8", "Invalid table"};
                }
                requestedTable = *parsed;
            }
        }

        Customer customer{*getFirstField(data, "name"),
                          *getFirstField(data, "phone"),
                          getFirstField(data, "email").value_or(""),
                          getFirstField(data, "preference").value_or("")};

        auto notes = getFirstField(data, "notes").value_or("");
        if (!sheet.updateReservationDetails(id,
                                            customer,
                                            *partySizeOpt,
                                            *timePoint,
                                            std::chrono::minutes(*durationMinutes),
                                            notes,
                                            requestedTable,
                                            tableSpecified)) {
            return {409, "text/plain; charset=utf-8", "Unable to update reservation"};
        }

        reservation = sheet.findReservationById(id);
        response.body = reservationToJson(*reservation);
        return response;
    }

    const std::string statusPrefix = "/api/reservations/";
    const std::string statusSuffix = "/status";
    if (request.method == "POST" && startsWith(request.path, statusPrefix) &&
        request.path.size() > statusPrefix.size() + statusSuffix.size() &&
        endsWith(request.path, statusSuffix)) {
        auto id = request.path.substr(statusPrefix.size(), request.path.size() - statusPrefix.size() - statusSuffix.size());
        auto data = parseFormEncoded(request.body);
        if (!hasField(data, "status")) {
            return {400, "text/plain; charset=utf-8", "Missing status"};
        }
        auto status = parseReservationStatus(*getFirstField(data, "status"));
        if (!status) {
            return {400, "text/plain; charset=utf-8", "Invalid status"};
        }
        auto reservation = sheet.findReservationById(id);
        if (!reservation) {
            return {404, "text/plain; charset=utf-8", "Reservation not found"};
        }
        switch (*status) {
            case ReservationStatus::Seated:
                reservation->markSeated();
                break;
            case ReservationStatus::Completed:
                reservation->markCompleted();
                break;
            case ReservationStatus::Cancelled:
                reservation->cancel();
                reservation->clearTable();
                break;
            case ReservationStatus::Open:
                reservation->updateStatus(ReservationStatus::Open);
                break;
        }
        response.body = "{\"success\":true}";
        return response;
    }

    const std::string tableSuffix = "/table";
    if (request.method == "POST" && startsWith(request.path, statusPrefix) &&
        request.path.size() > statusPrefix.size() + tableSuffix.size() && endsWith(request.path, tableSuffix)) {
        auto id = request.path.substr(statusPrefix.size(), request.path.size() - statusPrefix.size() - tableSuffix.size());
        auto reservation = sheet.findReservationById(id);
        if (!reservation) {
            return {404, "text/plain; charset=utf-8", "Reservation not found"};
        }
        auto data = parseFormEncoded(request.body);
        auto mode = getFirstField(data, "mode").value_or("");
        if (mode == "clear") {
            if (!sheet.clearTableAssignment(id)) {
                return {409, "text/plain; charset=utf-8", "Unable to clear table"};
            }
        } else if (mode == "auto") {
            if (!sheet.autoAssignTable(id)) {
                return {409, "text/plain; charset=utf-8", "No suitable table available"};
            }
        } else {
            auto tableField = getFirstField(data, "tableId");
            if (!tableField || tableField->empty()) {
                return {400, "text/plain; charset=utf-8", "Missing tableId"};
            }
            auto parsed = toInt(*tableField);
            if (!parsed || *parsed <= 0) {
                return {400, "text/plain; charset=utf-8", "Invalid tableId"};
            }
            if (!sheet.assignTable(id, *parsed)) {
                return {409, "text/plain; charset=utf-8", "Table not available"};
            }
        }
        reservation = sheet.findReservationById(id);
        if (!reservation) {
            return {404, "text/plain; charset=utf-8", "Reservation not found"};
        }
        response.body = reservationToJson(*reservation);
        return response;
    }

    return {404, "text/plain; charset=utf-8", "Not Found"};
}

int portableSend(SocketHandle socket, const char *data, size_t length) {
    size_t totalSent = 0;
    while (totalSent < length) {
#ifdef _WIN32
        SendSize chunk = send(socket, data + totalSent, static_cast<int>(length - totalSent), 0);
#else
        SendSize chunk = send(socket, data + totalSent, length - totalSent, 0);
#endif
        if (chunk <= 0) {
            return -1;
        }
        totalSent += static_cast<size_t>(chunk);
    }
    return static_cast<int>(totalSent);
}

void handleClient(SocketHandle clientFd,
                  Restaurant &restaurant,
                  std::mutex &mutex,
                  const std::string &staticRoot) {
    HttpRequest request;
    if (!readHttpRequest(clientFd, request)) {
        closeSocket(clientFd);
        return;
    }

    bool isApiRequest = startsWith(request.path, "/api/");

    HttpResponse response;
    if (request.method == "OPTIONS" && isApiRequest) {
        response = buildPreflightResponse();
    } else if (isApiRequest) {
        response = handleApiRequest(request, restaurant, mutex);
        applyCorsHeaders(response, true);
    } else {
        response = serveStaticFile(staticRoot, request.path);
        applyCorsHeaders(response, false);
    }

    auto text = buildResponse(response);
    portableSend(clientFd, text.c_str(), text.size());
    closeSocket(clientFd);
}

}  // namespace

SocketHandle createListeningSocket(int port) {
    SocketHandle serverFd = INVALID_SOCKET_HANDLE;

#ifdef AF_INET6
    serverFd = socket(AF_INET6, SOCK_STREAM, 0);
    if (serverFd != INVALID_SOCKET_HANDLE) {
#ifdef _WIN32
        char reuse = 1;
        setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        char dualStack = 0;
        setsockopt(serverFd, IPPROTO_IPV6, IPV6_V6ONLY, &dualStack, sizeof(dualStack));
#else
        int reuse = 1;
        setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        int dualStack = 0;
        setsockopt(serverFd, IPPROTO_IPV6, IPV6_V6ONLY, &dualStack, sizeof(dualStack));
#endif

        sockaddr_in6 address{};
        address.sin6_family = AF_INET6;
        address.sin6_addr = in6addr_any;
        address.sin6_port = htons(static_cast<uint16_t>(port));

        if (bind(serverFd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 0) {
            if (listen(serverFd, 10) == 0) {
                return serverFd;
            }
        }
        closeSocket(serverFd);
        serverFd = INVALID_SOCKET_HANDLE;
    }
#endif

    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == INVALID_SOCKET_HANDLE) {
        throw std::runtime_error("Failed to create socket");
    }

#ifdef _WIN32
    char opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#else
    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(serverFd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        closeSocket(serverFd);
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(serverFd, 10) < 0) {
        closeSocket(serverFd);
        throw std::runtime_error("Failed to listen on socket");
    }

    return serverFd;
}

void runWebServer(Restaurant &restaurant, const std::string &staticDir, int port) {
    [[maybe_unused]] SocketEnvironment socketEnv;

    SocketHandle serverFd = createListeningSocket(port);

    std::cout << "Web server running on http://localhost:" << port << "\n";
#ifdef AF_INET6
    std::cout << "Web server also available via http://[::1]:" << port << "\n";
#endif

    std::mutex mutex;
    while (true) {
        sockaddr_in clientAddress{};
        socklen_t clientLen = sizeof(clientAddress);
        SocketHandle clientFd = accept(serverFd, reinterpret_cast<sockaddr *>(&clientAddress), &clientLen);
        if (clientFd == INVALID_SOCKET_HANDLE) {
#ifdef _WIN32
            int error = WSAGetLastError();
            if (error == WSAEINTR) {
                continue;
            }
#else
            if (errno == EINTR) {
                continue;
            }
#endif
            continue;
        }
        std::thread worker(handleClient, clientFd, std::ref(restaurant), std::ref(mutex), staticDir);
        worker.detach();
    }
}

}  // namespace booking

