/*
 * dbus_client.c - DBus 客户端实现
 * 
 * 通过 sd-bus API 调用 dbus_service 暴露的方法。
 * server_dbus.c 使用这些函数，通过 DBus IPC 间接操作数据库，
 * 而不是直接调用 database_insert/database_select。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <systemd/sd-bus.h>
#include "dbus_client.h"
#include "dbus_service.h"

static sd_bus *bus = NULL;

int dbus_client_init(void) {
    int r;

    r = sd_bus_open_user(&bus);
    if (r < 0) {
        fprintf(stderr, "[DBus Client] Failed to connect to bus: %s\n", strerror(-r));
        return -1;
    }

    printf("[DBus Client] Connected to session bus\n");
    return 0;
}

void dbus_client_close(void) {
    if (bus != NULL) {
        sd_bus_unref(bus);
        bus = NULL;
        printf("[DBus Client] Disconnected from bus\n");
    }
}

int dbus_client_insert(const char *type, const char *data) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *reply = NULL;
    int r;
    int success = 0;

    /*
     * sd_bus_call_method: 同步调用远程方法
     * 参数: bus, 目标服务名, object path, 接口名, 方法名,
     *       错误输出, 返回消息, 输入签名, 输入参数...
     */
    r = sd_bus_call_method(bus,
                           DBUS_BUS_NAME,
                           DBUS_OBJECT_PATH,
                           DBUS_INTERFACE_NAME,
                           DBUS_METHOD_INSERT,
                           &error,
                           &reply,
                           "ss",        /* 输入: 两个字符串 */
                           type, data);
    if (r < 0) {
        fprintf(stderr, "[DBus Client] Insert call failed: %s\n", error.message);
        sd_bus_error_free(&error);
        return -1;
    }

    /* 读取返回值 */
    r = sd_bus_message_read(reply, "b", &success);
    if (r < 0) {
        fprintf(stderr, "[DBus Client] Failed to read reply: %s\n", strerror(-r));
        sd_bus_message_unref(reply);
        sd_bus_error_free(&error);
        return -1;
    }

    sd_bus_message_unref(reply);
    sd_bus_error_free(&error);

    return success ? 0 : -1;
}

int dbus_client_select(char *result, int result_size) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *reply = NULL;
    int r;
    int count = 0;
    const char *data = NULL;

    r = sd_bus_call_method(bus,
                           DBUS_BUS_NAME,
                           DBUS_OBJECT_PATH,
                           DBUS_INTERFACE_NAME,
                           DBUS_METHOD_SELECT,
                           &error,
                           &reply,
                           "");         /* 无输入参数 */
    if (r < 0) {
        fprintf(stderr, "[DBus Client] Select call failed: %s\n", error.message);
        sd_bus_error_free(&error);
        return -1;
    }

    /* 读取返回值: 整数 + 字符串 */
    r = sd_bus_message_read(reply, "is", &count, &data);
    if (r < 0) {
        fprintf(stderr, "[DBus Client] Failed to read reply: %s\n", strerror(-r));
        sd_bus_message_unref(reply);
        sd_bus_error_free(&error);
        return -1;
    }

    /* 复制结果到调用方的 buffer */
    if (data != NULL) {
        strncpy(result, data, result_size - 1);
        result[result_size - 1] = '\0';
    } else {
        result[0] = '\0';
    }

    sd_bus_message_unref(reply);
    sd_bus_error_free(&error);

    return count;
}
