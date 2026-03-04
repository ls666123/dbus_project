#ifndef DBUS_CLIENT_H
#define DBUS_CLIENT_H

/*
 * dbus_client.h - DBus 客户端接口
 * 
 * 提供通过 DBus 调用远程数据库操作的函数，
 * 供 server_dbus.c 使用，替代直接的 database_insert/database_select 调用。
 */

/* 初始化 DBus 连接 */
int dbus_client_init(void);

/* 关闭 DBus 连接 */
void dbus_client_close(void);

/* 通过 DBus 调用 Insert 方法 */
int dbus_client_insert(const char *type, const char *data);

/* 通过 DBus 调用 Select 方法 */
int dbus_client_select(char *result, int result_size);

#endif
