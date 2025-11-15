# 餐厅预订系统

本项目是一个使用 C++17 面向对象方法实现的简易餐厅预订与点餐管理系统。系统根据给定的用例图和分析类图构建，涵盖桌位管理、预订流程、散客处理、订单记录以及经营报表等核心功能。

## 功能概览

- **桌位管理**：维护桌位容量与状态，支持自动分配合适的桌位。
- **预订管理**：记录顾客信息、到店时间、预订备注，并支持状态变更（待到店、已入座、已完成、已取消）。
- **预订维护**：支持通过 HTTP API 查询单条预订详情、修改顾客信息/就餐时间/桌位安排，或直接删除（标记取消）。
- **散客处理**：快速为散客创建即时预订并尝试安排桌位。
- **点餐管理**：按预订记录订单及菜品，自动计算订单金额，并在前端实时展示。
- **经营报表**：生成当日预订数量、入座人数和营业收入的概览。
- **员工与权限**：区分前台与经理角色，体现权限控制模型。

## 代码结构

```
├── CMakeLists.txt
├── README.md
├── src
│   ├── ReservationSystem.hpp   // 系统核心类与数据结构声明
│   ├── ReservationSystem.cpp   // 核心逻辑实现
│   ├── SeedData.cpp/.hpp       // 示例基础数据装载
│   ├── WebServer.cpp/.hpp      // 极简 HTTP 服务端实现
│   ├── main.cpp                // 命令行界面入口
│   └── web_main.cpp            // Web 前端入口
└── web
    ├── index.html              // 单页前端界面
    ├── styles.css              // 界面样式
    └── app.js                  // 与后端交互的脚本
```

## 构建与运行

在首次尝试运行前，请确认已经安装：

- **CMake 3.16+**（Windows 用户建议使用 [CMake 官方安装包](https://cmake.org/download/) 并勾选 *Add CMake to system PATH*）。
- **C++17 编译器**：例如 Linux/macOS 上的 `g++`/`clang++`，或 Windows 上的 *Visual Studio 2019 及以上版本*（勾选“使用 C++ 的桌面开发”工作负载）。

随后根据需要选择命令行或图形界面方式进行构建运行。

### 方式一：命令行构建（跨平台）

```bash
# 1. 在仓库根目录执行（Windows PowerShell 亦可）
cmake -S . -B build

# 2. 编译生成可执行文件
cmake --build build

# 3a. 运行命令行版本
./build/restaurant_booking            # Windows 上改为 .\build\Debug\restaurant_booking.exe

# 3b. 运行 Web 服务并访问前端（默认端口 8080）
./build/restaurant_booking_server     # Windows 上改为 .\build\Debug\restaurant_booking_server.exe
```

> 💡 如果在 Windows 上看到“无法启动程序，因为缺少 MSVCP***.dll”之类的提示，请使用 **x64 Native Tools Command Prompt for VS** 或 VS Developer PowerShell 重新执行以上命令，即可自动使用 Visual Studio 的编译器与运行时。

命令行程序启动后将显示交互式菜单，可根据提示进行桌位查询、创建预订、处理散客、录入点餐以及生成报表等操作。

若运行 Web 服务，请在启动后于浏览器访问 `http://localhost:8080`（或你指定的端口）。静态资源默认来自仓库的 `web/` 目录，也可以通过命令行参数调整：

```bash
# 自定义端口与静态目录示例
./build/restaurant_booking_server 9000 ./web
```

Web 版本会在本地启动一个极简的 HTTP 服务，默认挂载 `web/` 目录下的静态前端页面。启动后在浏览器访问 `http://localhost:8080` 即可看到可视化界面：

- **桌位概览**：以卡片形式展示当前桌位状态。
- **预订列表**：展示所有预订并支持一键切换状态（入座/完成/取消）。
- **预订维护操作**：在列表中直接删除预订，或通过 API 更新客人信息、就餐时间与桌位；悬停可查看预计结束时间与最近更新时间。
- **创建预订/散客登记**：通过表单向后台提交数据，成功后即时刷新视图。
- **点餐与订单列表**：支持为指定预订录入菜品数量，服务端计算订单金额并展示订单明细。
- **菜单与经营数据**：实时拉取示例菜单与当日报表，方便课堂演示。
- **员工信息**：展示种子数据中配置的前台与经理账号，呼应权限模型。

前端仅作为课程作业的演示界面，所有数据仍存放在内存中，重启服务后会回到初始状态。

### 方式二：使用 Visual Studio（Windows）

1. 打开 **x64 Native Tools Command Prompt for VS**，执行 `cmake -S . -B build -G "Visual Studio 17 2022"` 生成解决方案文件。
2. 用 Visual Studio 打开 `build/restaurant_booking.sln`。
3. 选择需要的启动项目：
   - `restaurant_booking`：命令行界面。
   - `restaurant_booking_server`：带前端的 HTTP 服务。
4. 直接按 **F5**/“本地 Windows 调试器”即可试运行。若选择 Web 服务，记得在浏览器访问 `http://localhost:8080`。

这样即可在熟悉的 IDE 中编译、调试与演示系统。

- **HTTP API 拓展**：
  - `GET /api/reservations/{id}`：返回单条预订的完整详情（顾客信息、时间、桌位、状态、最后更新时间等）。
  - `PUT /api/reservations/{id}`：使用 `application/x-www-form-urlencoded` 提交字段以更新顾客信息、就餐时间、时长、备注与（可选）桌位。
  - `DELETE /api/reservations/{id}`：将预订标记为取消并释放桌位。
