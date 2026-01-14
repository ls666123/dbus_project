#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8888
#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUF_SIZE];
    
    const char *server_ip = (argc > 1) ? argv[1] : "127.0.0.1";
    
    // 创建 socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket failed");
        exit(1);
    }
    
    // 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(1);
    }
    
    // 连接服务器
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        exit(1);
    }
    
    printf("[Client] Connected to server %s:%d\n", server_ip, PORT);
    printf("[Client] Enter message to send (type 'quit' to exit):\n");
    
    // 从 console 输入数据并发送
    while (1) {
        printf("> ");
        fflush(stdout);
        
        // 从 console 读取输入
        if (fgets(buffer, BUF_SIZE, stdin) == NULL) {
            break;
        }
        
        // 去掉换行符
        buffer[strcspn(buffer, "\n")] = 0;
        
        // 退出
        if (strcmp(buffer, "quit") == 0) {
            printf("[Client] Exiting...\n");
            break;
        }
        
        // 跳过空输入
        if (strlen(buffer) == 0) {
            continue;
        }
        
        // 发送数据
        //send(sock, buffer, strlen(buffer), 0);
        ssize_t sent = send(sock, buffer, strlen(buffer), 0);
        if (sent < 0) {
            perror("[Client] send error");
            break;
        }
        
        // 接收响应
        //memset(buffer, 0, BUF_SIZE);
        //int n = recv(sock, buffer, BUF_SIZE - 1, 0);
        ssize_t n = recv(sock, buffer, BUF_SIZE - 1, 0);
        if (n <= 0) {
            printf("[Client] Server disconnected\n");
            break;
        }
        buffer[n] = '\0';
        printf("[Client] Response (%zd bytes): %s\n", n, buffer);
        //printf("[Client] %s\n", buffer);
    }
    close(sock);
    return 0;
}
