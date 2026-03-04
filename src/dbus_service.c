

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <systemd/sd-bus.h>
#include "database.h"
#include "dbus_service.h"

static volatile sig_atomic_t running = 1;

/* 信号处理 */
static void signal_handler(int sig) {
    (void)sig;
    running = 0;
}

/*
 * DBus 方法处理: Insert
 * 输入参数: ss (type, data)  两个字符串
 * 输出参数: b (success)      一个布尔值
 */
static int method_insert(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    (void)userdata;
    (void)ret_error;

    const char *type = NULL;
    const char *data = NULL;
    int r;

    /* 读取输入参数 */
    r = sd_bus_message_read(m, "ss", &type, &data);
    if (r < 0) {
        fprintf(stderr, "[DBus Service] Failed to read parameters: %s\n", strerror(-r));
        return r;
    }

    printf("[DBus Service] Insert called: type=%s, data=%s\n", type, data);

    /* 调用数据库 insert */
    int result = database_insert(type, data);

    /* 返回结果 */
    return sd_bus_reply_method_return(m, "b", result == 0);
}

/*
 * DBus 方法处理: Select
 * 输入参数: 无
 * 输出参数: is (count, result_string)  一个整数和一个字符串
 */
static int method_select(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    (void)userdata;
    (void)ret_error;

    printf("[DBus Service] Select called\n");

    char result[4096];
    int count = database_select(result, sizeof(result));

    if (count < 0) {
        count = 0;
        result[0] = '\0';
    }

    /* 返回记录数和查询结果字符串 */
    return sd_bus_reply_method_return(m, "is", count, result);
}

/*
 * vtable: 定义 DBus 接口的方法列表
 * SD_BUS_METHOD(方法名, 输入签名, 输出签名, 处理函数, 标志)
 */
static const sd_bus_vtable service_vtable[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD(DBUS_METHOD_INSERT, "ss", "b", method_insert, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD(DBUS_METHOD_SELECT, "", "is", method_select, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_VTABLE_END
};

int main(void) {
    sd_bus_slot *slot = NULL;
    sd_bus *bus = NULL;
    int r;

    /* 设置信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* 初始化数据库 */
    if (database_init("messages.db") < 0) {
        fprintf(stderr, "[DBus Service] Failed to init database\n");
        return 1;
    }

    /* 连接到 session bus */
    r = sd_bus_open_user(&bus);
    if (r < 0) {
        fprintf(stderr, "[DBus Service] Failed to connect to bus: %s\n", strerror(-r));
        database_close();
        return 1;
    }

    /* 注册 vtable（将方法挂载到 object path 上） */
    r = sd_bus_add_object_vtable(bus,
                                  &slot,
                                  DBUS_OBJECT_PATH,
                                  DBUS_INTERFACE_NAME,
                                  service_vtable,
                                  NULL);
    if (r < 0) {
        fprintf(stderr, "[DBus Service] Failed to add vtable: %s\n", strerror(-r));
        sd_bus_unref(bus);
        database_close();
        return 1;
    }

    /* 请求 bus name */
    r = sd_bus_request_name(bus, DBUS_BUS_NAME, 0);
    if (r < 0) {
        fprintf(stderr, "[DBus Service] Failed to acquire name: %s\n", strerror(-r));
        sd_bus_slot_unref(slot);
        sd_bus_unref(bus);
        database_close();
        return 1;
    }

    printf("[DBus Service] Running on session bus\n");
    printf("[DBus Service] Bus name: %s\n", DBUS_BUS_NAME);
    printf("[DBus Service] Object:   %s\n", DBUS_OBJECT_PATH);
    printf("[DBus Service] Interface: %s\n", DBUS_INTERFACE_NAME);
    printf("[DBus Service] Press Ctrl+C to quit.\n");

    /* 主事件循环 */
    while (running) {
        /* 处理 DBus 消息，超时 1 秒 */
        r = sd_bus_process(bus, NULL);
        if (r < 0) {
            fprintf(stderr, "[DBus Service] Process failed: %s\n", strerror(-r));
            break;
        }

        /* 如果还有待处理的消息，继续处理 */
        if (r > 0)
            continue;

        /* 没有消息，等待下一条 */
        r = sd_bus_wait(bus, (uint64_t)1000000);  /* 1 秒超时 */
        if (r < 0 && r != -EINTR) {
            fprintf(stderr, "[DBus Service] Wait failed: %s\n", strerror(-r));
            break;
        }
    }

    /* 清理 */
    printf("\n[DBus Service] Shutting down...\n");
    sd_bus_release_name(bus, DBUS_BUS_NAME);
    sd_bus_slot_unref(slot);
    sd_bus_unref(bus);
    database_close();
    printf("[DBus Service] Goodbye!\n");

    return 0;
}
