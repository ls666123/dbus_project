# Linux 基于 DBus 的多线程通信系统
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
