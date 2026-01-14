# Linux 基于 DBus 的多线程通信系统

## 第一部分：C语言 Socket 编程
- TCP 协议实现客户端和服务器端
- 服务器端支持多线程处理多个客户端
- 客户端从 console 输入数据，服务器接收并打印

## 第二部分：C语言操作数据库（sqlite3）
- 实现 `database_init()` - 初始化数据库
- 实现 `database_close()` - 关闭数据库
- 实现 `database_insert()` - 插入数据
- 实现 `database_select()` - 查询数据

### 数据库表结构
| 字段 | 类型 | 说明 |
|------|------|------|
| id | INTEGER | 主键自增 |
| type | TEXT | 消息类型 (Receive/Return) |
| data | TEXT | 消息内容 |
| timestamp | TEXT | 时间戳 |

## 编译
```bash
sudo apt install -y libsqlite3-dev
make
```

## 运行
```bash
# 测试数据库功能
./test_database

# 测试 Socket 通信
./server    # 终端1
./client    # 终端2
```
