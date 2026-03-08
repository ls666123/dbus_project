# Linux 基于 DBus 的多线程通信系统

## 项目结构

```
dbus_project/
├── src/
│   ├── server.c          # 第一~三部分: Socket 服务器（直连数据库）
│   ├── client.c          # 第一部分: Socket 客户端
│   ├── database.c        # 第二部分: SQLite3 数据库操作
│   ├── database.h        # 数据库接口头文件
│   ├── dbus_service.c    # 加分项: DBus 数据库服务进程
│   ├── dbus_service.h    # DBus 接口定义（bus name, path, interface）
│   ├── dbus_client.c     # 加分项: DBus 客户端封装
│   ├── dbus_client.h     # DBus 客户端接口头文件
│   ├── server_dbus.c     # 加分项: 使用 DBus IPC 的 Socket 服务器
│   └── test_database.c   # 数据库测试
├── uart_ui.sh            # 第四部分: Dialog Shell UI
├── Makefile
└── README.md
```

## 第一部分：C语言 Socket 编程

- TCP 协议实现客户端和服务器端
- 服务器端支持多线程处理多个客户端
- 客户端从 console 输入数据，服务器接收并打印

## 第二部分：C语言操作数据库（sqlite3）

- `database_init()` - 初始化数据库
- `database_close()` - 关闭数据库
- `database_insert()` - 插入数据
- `database_select()` - 查询数据
- `database_clear()` - 清空数据
- `database_count()` - 统计记录数

支持独立测试模式：

```bash
./database test      # 运行测试
./database select    # 查询记录
./database count     # 统计数量
```

## 第三部分：服务器集成数据库

- 服务器收到客户端数据时，调用 `database_insert("Receive", data)` 存储
- 服务器返回响应时，调用 `database_insert("Return", response)` 存储
- 客户端可输入 `query` 命令查询数据库记录

## 加分项：DBus IPC

使用 systemd 的 sd-bus API 实现进程间通信，将数据库操作从 Socket 服务器分离到独立的 DBus 服务进程中。

### 架构

```
client <--TCP Socket--> server_dbus <--DBus IPC--> dbus_service <--sqlite3--> database
```

### 核心文件说明

| 文件 | 说明 |
|------|------|
| `dbus_service.h` | 定义 DBus 接口常量（Bus Name、Object Path、Interface） |
| `dbus_service.c` | DBus 服务端进程，拥有数据库，暴露 `Insert` 和 `Select` 方法 |
| `dbus_client.c` | DBus 客户端封装，提供 `dbus_client_insert()` 和 `dbus_client_select()` |
| `server_dbus.c` | Socket 服务器，通过 DBus 调用 dbus_service 来操作数据库 |

### DBus 接口定义

```
Bus Name:   com.dbus.project.Database
Object Path: /com/dbus/project/Database
Interface:   com.dbus.project.DatabaseInterface

Methods:
  Insert(type: string, data: string) → (success: boolean)
  Select() → (count: int, result: string)
```

### 编译和运行

```bash
# 安装依赖
sudo apt install libsystemd-dev libsqlite3-dev

# 编译 DBus 相关程序
make dbus

# 步骤1: 启动 DBus 数据库服务
./dbus_service

# 步骤2: 在另一个终端启动使用 DBus 的服务器
./server_dbus

# 步骤3: 在另一个终端启动客户端
./client
```

## 第四部分：Dialog Shell UI

使用 `dialog` 工具绘制终端 UI 界面，提供两个功能：
1. 从 UI 输入数据发送给 Socket 服务器
2. 查询 database 中的数据

### 运行

```bash
# 安装依赖
sudo apt install dialog netcat-openbsd

# 确保服务器已启动（使用原版或 DBus 版均可）
# 方式A（原版）:
./server

# 方式B（DBus 版）:
./dbus_service &    # 后台启动 DBus 服务
./server_dbus       # 启动 DBus 版服务器

# 启动 Dialog UI
./uart_ui.sh
# 或指定服务器 IP:
./uart_ui.sh 192.168.1.100
```

### UI 截图说明

- 主菜单: 选择 "Send data to server" 或 "Query database"
- 发送数据: 输入框输入数据 → 通过 Socket 发送 → 显示服务器响应
- 查询数据: 自动发送查询命令 → 显示数据库记录列表

## 完整编译指令

```bash
# 编译全部（第一~三部分）
make all

# 编译 DBus 加分项
make dbus

# 编译所有
make all dbus

# 清理
make clean
```# Linux 基于 DBus 的多线程通信系统
## 第一部分：C语言 Socket 编程

- TCP 协议实现客户端和服务器端
- 服务器端支持多线程处理多个客户端
- 客户端从 console 输入数据，服务器接收并打印

## 第二部分：C语言操作数据库（sqlite3）

- `database_init()` - 初始化数据库
- `database_close()` - 关闭数据库
- `database_insert()` - 插入数据
- `database_select()` - 查询数据
- `database_clear()` - 清空数据
- `database_count()` - 统计记录数

支持独立测试模式：
```bash
./database test      # 运行测试
./database select    # 查询记录
./database count     # 统计数量
## 第三部分：服务器集成数据库

- 服务器收到客户端数据时，调用 `database_insert("Receive", data)` 存储
- 服务器返回响应时，调用 `database_insert("Return", response)` 存储
- 客户端可输入 `query` 命令查询数据库记录
## 第四部分：Dialog Shell UI（待完成）

- 使用 Dialog 绘制 UART UI
- 从 UI 输入数据给 Socket 客户端
- 查询 database 数据

## 加分项：DBus IPC
使用 systemd 的 sd-bus API 实现进程间通信，将数据库操作从 Socket 服务器分离到独立的 DBus 服务进程中。
