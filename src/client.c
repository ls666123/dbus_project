#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8888
#define BUF_SIZE 4096

void print_help(void) {
    printf("\nCommands:\n");
    printf("  <message>  - Send message to server\n");
    printf("  query      - Query database records\n");
    printf("  quit       - Exit client\n\n");
}

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUF_SIZE];

    const char *server_ip = (argc > 1) ? argv[1] : "127.0.0.1";

    /* 创建 socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[Client] socket failed");
        return 1;
    }

    /* 设置服务器地址 */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("[Client] Invalid address");
        close(sock);
        return 1;
    }

    /* 连接服务器 */
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("[Client] connect failed");
        close(sock);
        return 1;
    }

    printf("[Client] Connected to %s:%d\n", server_ip, PORT);
    print_help();

    /* 主循环 */
    while (1) {
        printf("> ");
        fflush(stdout);

        /* 读取用户输入 */
        if (fgets(buffer, BUF_SIZE, stdin) == NULL) {
            break;
        }

        /* 去掉换行符 */
        buffer[strcspn(buffer, "\n")] = '\0';

        /* 退出命令 */
        if (strcmp(buffer, "quit") == 0) {
            printf("[Client] Bye!\n");
            break;
        }

        /* 跳过空输入 */
        if (strlen(buffer) == 0) {
            continue;
        }

        /* 查询命令转换 */
        if (strcmp(buffer, "query") == 0) {
            strcpy(buffer, "CMD:QUERY");
        }

        /* 发送数据 */
        ssize_t sent = send(sock, buffer, strlen(buffer), 0);
        if (sent < 0) {
            perror("[Client] send failed");
            break;
        }

        /* 接收响应 */
        ssize_t n = recv(sock, buffer, BUF_SIZE - 1, 0);
        if (n < 0) {
            perror("[Client] recv failed");
            break;
        }

        if (n == 0) {
            printf("[Client] Server disconnected\n");
            break;
        }

        /* 关键：手动补 \0 */
        buffer[n] = '\0';

        printf("[Server] %s\n", buffer);
    }

    close(sock);
    return 0;
}
