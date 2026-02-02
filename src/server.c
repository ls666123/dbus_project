#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "database.h"

#define PORT 8888
#define BUF_SIZE 4096

/* 数据库互斥锁 */
static pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER;

/* 运行标志 */
static volatile sig_atomic_t running = 1;

/* 信号处理 */
void signal_handler(int sig) {
    (void)sig;
    printf("\n[Server] Shutting down...\n");
    running = 0;
}

/* 客户端处理线程 */
void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);
    char buffer[BUF_SIZE];

    printf("[Server] Thread started for client fd=%d\n", client_fd);

    while (running) {
        /* 接收数据 */
        ssize_t n = recv(client_fd, buffer, BUF_SIZE - 1, 0);

        if (n < 0) {
            perror("[Server] recv error");
            break;
        }

        if (n == 0) {
            printf("[Server] Client disconnected, fd=%d\n", client_fd);
            break;
        }

        /*  \0，将字节流转为字符串 */
        buffer[n] = '\0';

        printf("[Server] Received (%zd bytes): %s\n", n, buffer);

        /* 处理查询命令 */
        if (strcmp(buffer, "CMD:QUERY") == 0) {
            char result[BUF_SIZE];

            pthread_mutex_lock(&db_mutex);
            int count = database_select(result, sizeof(result));
            pthread_mutex_unlock(&db_mutex);

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

        /* 存入数据库（一次加锁完成两次操作） */
        pthread_mutex_lock(&db_mutex);
        database_insert("Receive", buffer);
        database_insert("Return", response);
        pthread_mutex_unlock(&db_mutex);

        /* 发送响应 */
        send(client_fd, response, strlen(response), 0);
    }

    close(client_fd);
    printf("[Server] Thread exited, fd=%d\n", client_fd);
    return NULL;
}

int main(void) {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    /* 设置信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* 初始化数据库 */
    if (database_init("messages.db") < 0) {
        fprintf(stderr, "[Server] Failed to init database\n");
        return 1;
    }

    /* 创建 socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("[Server] socket failed");
        database_close();
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
        perror("[Server] bind failed");
        database_close();
        close(server_fd);
        return 1;
    }

    /* 监听 */
    if (listen(server_fd, 5) < 0) {
        perror("[Server] listen failed");
        database_close();
        close(server_fd);
        return 1;
    }

    printf("[Server] Listening on port %d...\n", PORT);
    printf("[Server] Database ready. Press Ctrl+C to quit.\n");

    /* 主循环 */
    while (running) {
        int *client_fd = malloc(sizeof(int));
        if (client_fd == NULL) {
            perror("[Server] malloc failed");
            continue;
        }

        *client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

        if (*client_fd < 0) {
            if (running) {
                perror("[Server] accept failed");
            }
            free(client_fd);
            continue;
        }

        printf("[Server] Client connected: %s:%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        /* 创建线程 */
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, client_fd) != 0) {
            perror("[Server] pthread_create failed");
            close(*client_fd);
            free(client_fd);
            continue;
        }
        pthread_detach(tid);
    }

    /* 清理 */
    printf("[Server] Cleaning up...\n");
    database_close();
    close(server_fd);
    printf("[Server] Goodbye!\n");

    return 0;
}
