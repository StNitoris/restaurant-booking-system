#include "ReservationSystem.hpp"
#include "SeedData.hpp"

#include <iostream>
#include <limits>

using booking::BookingSheet;
using booking::Customer;
using booking::FrontDeskStaff;
using booking::Manager;
using booking::MenuItem;
using booking::Reservation;
using booking::ReservationStatus;
using booking::Restaurant;
using booking::Table;
using booking::seedRestaurant;

namespace {

constexpr int kDefaultDurationMinutes = 120;

void showTables(const Restaurant &restaurant) {
    std::cout << "当前桌位状态:\n";
    for (const auto &table : restaurant.getBookingSheet().getTables()) {
        std::cout << "  桌号" << table.getId() << " (" << table.getCapacity() << "人, " << table.getLocation()
                  << ") 状态:";
        switch (table.getStatus()) {
            case booking::TableStatus::Free:
                std::cout << "空闲";
                break;
            case booking::TableStatus::Reserved:
                std::cout << "已预订";
                break;
            case booking::TableStatus::Occupied:
                std::cout << "用餐中";
                break;
            case booking::TableStatus::OutOfService:
                std::cout << "暂停使用";
                break;
        }
        std::cout << '\n';
    }
}

void listReservations(const Restaurant &restaurant) {
    const auto &reservations = restaurant.getBookingSheet().getReservations();
    if (reservations.empty()) {
        std::cout << "暂无预订记录。\n";
        return;
    }
    std::cout << "所有预订:\n";
    for (const auto &reservation : reservations) {
        std::cout << "  编号:" << reservation.getId() << " 客人:" << reservation.getCustomer().getName()
                  << " 人数:" << reservation.getPartySize() << " 时间:"
                  << booking::formatDateTime(reservation.getDateTime());
        if (reservation.getTableId()) {
            std::cout << " 桌号:" << *reservation.getTableId();
        }
        std::cout << " 状态:";
        switch (reservation.getStatus()) {
            case ReservationStatus::Open:
                std::cout << "待到店";
                break;
            case ReservationStatus::Seated:
                std::cout << "已入座";
                break;
            case ReservationStatus::Completed:
                std::cout << "已完成";
                break;
            case ReservationStatus::Cancelled:
                std::cout << "已取消";
                break;
        }
        std::cout << '\n';
    }
}

int readInt(const std::string &prompt) {
    int value;
    while (true) {
        std::cout << prompt;
        if (std::cin >> value) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "输入无效，请重新输入。\n";
    }
}

std::string readLine(const std::string &prompt) {
    std::cout << prompt;
    std::string input;
    std::getline(std::cin, input);
    return input;
}

void createReservationFlow(Restaurant &restaurant) {
    std::string name = readLine("顾客姓名: ");
    std::string phone = readLine("联系方式: ");
    std::string email = readLine("电子邮箱(可选): ");
    std::string preference = readLine("口味偏好(可选): ");
    int partySize = readInt("就餐人数: ");
    std::string timeText = readLine("预订时间(格式 YYYY-MM-DD HH:MM): ");
    auto timePoint = booking::parseDateTime(timeText);
    if (!timePoint) {
        std::cout << "时间格式不正确。\n";
        return;
    }
    std::string notes = readLine("备注(可选): ");

    auto &sheet = restaurant.getBookingSheet();
    auto tableIds = sheet.findAllAvailableTableIds(partySize, *timePoint, std::chrono::minutes(kDefaultDurationMinutes));
    if (tableIds.empty()) {
        std::cout << "无可用桌位，预订失败。\n";
        return;
    }
    std::cout << "可用桌位:";
    for (int id : tableIds) {
        std::cout << ' ' << id;
    }
    std::cout << "\n";

    Customer customer{name, phone, email, preference};
    auto &reservation = sheet.createReservation(customer, partySize, *timePoint,
                                                std::chrono::minutes(kDefaultDurationMinutes), notes);
    if (reservation.getTableId()) {
        std::cout << "预订成功，分配桌号 " << *reservation.getTableId() << " ，编号 " << reservation.getId() << "。\n";
    } else {
        std::cout << "预订已创建，但暂未分配桌位，编号 " << reservation.getId() << "。\n";
    }
}

void walkInFlow(Restaurant &restaurant) {
    std::string name = readLine("顾客姓名: ");
    std::string phone = readLine("联系方式: ");
    int partySize = readInt("就餐人数: ");
    std::string notes = readLine("备注(可选): ");

    Customer customer{name, phone};
    auto &reservation = restaurant.getBookingSheet().recordWalkIn(customer, partySize, notes);
    if (reservation.getTableId()) {
        std::cout << "已为散客安排桌号 " << *reservation.getTableId() << " ，预订编号 " << reservation.getId()
                  << "。\n";
    } else {
        std::cout << "已登记散客，暂无法安排桌位。编号 " << reservation.getId() << "。\n";
    }
}

void updateReservationStatus(Restaurant &restaurant, ReservationStatus status, const std::string &actionText) {
    std::string id = readLine("输入预订编号: ");
    auto reservation = restaurant.getBookingSheet().findReservationById(id);
    if (!reservation) {
        std::cout << "未找到对应预订。\n";
        return;
    }
    switch (status) {
        case ReservationStatus::Seated:
            reservation->markSeated();
            break;
        case ReservationStatus::Completed:
            reservation->markCompleted();
            break;
        case ReservationStatus::Cancelled:
            reservation->cancel();
            break;
        default:
            reservation->updateStatus(status);
            break;
    }
    std::cout << actionText << "成功。\n";
}

void recordOrderFlow(Restaurant &restaurant) {
    std::string reservationId = readLine("预订编号: ");
    auto reservation = restaurant.getBookingSheet().findReservationById(reservationId);
    if (!reservation) {
        std::cout << "未找到预订。\n";
        return;
    }
    auto &order = restaurant.getBookingSheet().recordOrder(reservationId);
    std::cout << "开始录入点餐，订单编号 " << order.getId() << "。输入空行结束。\n";
    while (true) {
        std::string itemName = readLine("菜品名称: ");
        if (itemName.empty()) {
            break;
        }
        const auto *item = restaurant.findMenuItem(itemName);
        if (!item) {
            std::cout << "菜单中不存在该菜品。\n";
            continue;
        }
        int quantity = readInt("数量: ");
        order.addItem(*item, quantity);
    }
    std::cout << "订单总计: " << booking::formatCurrency(order.calculateTotal()) << "。\n";
}

void showMenu(const Restaurant &restaurant) {
    std::cout << "菜单列表:\n";
    for (const auto &item : restaurant.getMenu()) {
        std::cout << "  - " << item.getCategory() << ": " << item.getName() << " 价格 "
                  << booking::formatCurrency(item.getPrice()) << '\n';
    }
}

void showStaff(const Restaurant &restaurant) {
    std::cout << "员工列表:\n";
    for (const auto &staff : restaurant.getStaff()) {
        std::cout << "  - " << staff->getRole().getName() << " " << staff->getName() << " 联系方式 "
                  << staff->getContact() << '\n';
    }
}

void generateReport(const Restaurant &restaurant) {
    auto report = restaurant.generateDailyReport();
    std::cout << report.summary();
}

void refreshStatus(Restaurant &restaurant) {
    restaurant.getBookingSheet().updateTableStatuses();
}

void displayMenu() {
    std::cout << "\n===== 餐厅预订系统 =====\n";
    std::cout << "1. 查看桌位状态\n";
    std::cout << "2. 查看全部预订\n";
    std::cout << "3. 创建新预订\n";
    std::cout << "4. 处理散客\n";
    std::cout << "5. 标记预订为已入座\n";
    std::cout << "6. 标记预订为已完成\n";
    std::cout << "7. 取消预订\n";
    std::cout << "8. 录入点餐\n";
    std::cout << "9. 查看菜单\n";
    std::cout << "10. 查看员工\n";
    std::cout << "11. 生成经营报表\n";
    std::cout << "0. 退出\n";
    std::cout << "请选择操作: ";
}

}  // namespace

int main() {
    Restaurant restaurant{"美味餐厅", "上海市黄浦区中山东一路12号", BookingSheet{"2024-05-20"}};
    seedRestaurant(restaurant);

    bool running = true;
    while (running) {
        refreshStatus(restaurant);
        displayMenu();
        int choice;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "输入无效，请重新输入。\n";
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
            case 1:
                showTables(restaurant);
                break;
            case 2:
                listReservations(restaurant);
                break;
            case 3:
                createReservationFlow(restaurant);
                break;
            case 4:
                walkInFlow(restaurant);
                break;
            case 5:
                updateReservationStatus(restaurant, ReservationStatus::Seated, "入座操作");
                break;
            case 6:
                updateReservationStatus(restaurant, ReservationStatus::Completed, "完成操作");
                break;
            case 7:
                updateReservationStatus(restaurant, ReservationStatus::Cancelled, "取消操作");
                break;
            case 8:
                recordOrderFlow(restaurant);
                break;
            case 9:
                showMenu(restaurant);
                break;
            case 10:
                showStaff(restaurant);
                break;
            case 11:
                generateReport(restaurant);
                break;
            case 0:
                running = false;
                break;
            default:
                std::cout << "未知操作。\n";
                break;
        }
    }

    std::cout << "感谢使用，再见！\n";
    return 0;
}

