/*
 * server_dbus.c - 使用 DBus IPC 的 Socket 服务器
 * 
 * 与原始 server.c 功能相同，但数据库操作通过 DBus 调用
 * dbus_service 进程来完成，实现了进程间通信 (IPC)。
 *
 * 架构:
 *   client <--TCP--> server_dbus <--DBus IPC--> dbus_service <--sqlite3--> database
 *
 * 编译: gcc -Wall -g -pthread -o server_dbus src/server_dbus.c src/dbus_client.c \
 *        $(pkg-config --cflags --libs libsystemd) -lpthread
 * 运行: 先启动 ./dbus_service，再启动 ./server_dbus
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "dbus_client.h"

#define PORT 8888
#define BUF_SIZE 4096

/* DBus 调用互斥锁（sd-bus 不是线程安全的） */
static pthread_mutex_t dbus_mutex = PTHREAD_MUTEX_INITIALIZER;

/* 运行标志 */
static volatile sig_atomic_t running = 1;

/* 信号处理 */
void signal_handler(int sig) {
    (void)sig;
    printf("\n[Server-DBus] Shutting down...\n");
    running = 0;
}

/* 客户端处理线程 */
void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);
    char buffer[BUF_SIZE];

    printf("[Server-DBus] Thread started for client fd=%d\n", client_fd);

    while (running) {
        /* 接收数据 */
        ssize_t n = recv(client_fd, buffer, BUF_SIZE - 1, 0);

        if (n < 0) {
            perror("[Server-DBus] recv error");
            break;
        }

        if (n == 0) {
            printf("[Server-DBus] Client disconnected, fd=%d\n", client_fd);
            break;
        }

        buffer[n] = '\0';
        printf("[Server-DBus] Received (%zd bytes): %s\n", n, buffer);

        /* 处理查询命令 */
        if (strcmp(buffer, "CMD:QUERY") == 0) {
            char result[BUF_SIZE];

            /* 通过 DBus 查询数据库 */
            pthread_mutex_lock(&dbus_mutex);
            int count = dbus_client_select(result, sizeof(result));
            pthread_mutex_unlock(&dbus_mutex);

            char response[BUF_SIZE];
            if (count > 0) {
                snprintf(response, sizeof(response),
                         "\n=== Database Records (%d) ===\n"
                         "ID|Type|Data|Timestamp\n"
                         "----------------------------\n%s",
                         count, result);
            } else {
                snprintf(response, sizeof(response), "No records found.");
            }

            send(client_fd, response, strlen(response), 0);
            continue;
        }

        /* 构造响应 */
        char response[BUF_SIZE];
        snprintf(response, sizeof(response), "Server received: %s", buffer);

        /* 通过 DBus IPC 存入数据库 */
        pthread_mutex_lock(&dbus_mutex);
        dbus_client_insert("Receive", buffer);
        dbus_client_insert("Return", response);
        pthread_mutex_unlock(&dbus_mutex);

        /* 发送响应 */
        send(client_fd, response, strlen(response), 0);
    }

    close(client_fd);
    printf("[Server-DBus] Thread exited, fd=%d\n", client_fd);
    return NULL;
}

int main(void) {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    /* 设置信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* 初始化 DBus 连接 */
    if (dbus_client_init() < 0) {
        fprintf(stderr, "[Server-DBus] Failed to init DBus connection\n");
        fprintf(stderr, "[Server-DBus] Please make sure dbus_service is running!\n");
        return 1;
    }

    /* 创建 socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("[Server-DBus] socket failed");
        dbus_client_close();
        return 1;
    }

    /* 设置端口复用 */
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* 绑定地址 */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("[Server-DBus] bind failed");
        dbus_client_close();
        close(server_fd);
        return 1;
    }

    /* 监听 */
    if (listen(server_fd, 5) < 0) {
        perror("[Server-DBus] listen failed");
        dbus_client_close();
        close(server_fd);
        return 1;
    }

    printf("[Server-DBus] Listening on port %d...\n", PORT);
    printf("[Server-DBus] Using DBus IPC for database operations.\n");
    printf("[Server-DBus] Press Ctrl+C to quit.\n");

    /* 主循环 */
    while (running) {
        int *client_fd = malloc(sizeof(int));
        if (client_fd == NULL) {
            perror("[Server-DBus] malloc failed");
            continue;
        }

        *client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

        if (*client_fd < 0) {
            if (running) {
                perror("[Server-DBus] accept failed");
            }
            free(client_fd);
            continue;
        }

        printf("[Server-DBus] Client connected: %s:%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        /* 创建线程 */
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, client_fd) != 0) {
            perror("[Server-DBus] pthread_create failed");
            close(*client_fd);
            free(client_fd);
            continue;
        }
        pthread_detach(tid);
    }

    /* 清理 */
    printf("[Server-DBus] Cleaning up...\n");
    dbus_client_close();
    close(server_fd);
    printf("[Server-DBus] Goodbye!\n");

    return 0;
}
