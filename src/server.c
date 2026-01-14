#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8888
#define BUF_SIZE 1024

// 客户端处理线程
void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);
    char buffer[BUF_SIZE];
    
    printf("[Server] New client thread started, fd=%d\n", client_fd);
    
    while (1) {
        //memset(buffer, 0, BUF_SIZE);
        //int n = recv(client_fd, buffer, BUF_SIZE - 1, 0);
        ssize_t n = recv(client_fd, buffer, BUF_SIZE - 1, 0);
        
        if (n < 0) {
            perror("[Server] recv error");
            break;
        }
        
        if (n == 0) {
            printf("[Server] Client disconnected (graceful), fd=%d\n", client_fd);
            break;
        }
        
        // 打印收到的数据
        printf("[Server] Received %zd bytes from client: %s\n", n, buffer);
        
        // 返回响应
        char response[BUF_SIZE];
        int resp_len = snprintf(response, sizeof(response), "Server received: %s", buffer);
        send(client_fd, response, resp_len, 0);
    }
    
    close(client_fd);
    return NULL;
}

int main() {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    // 创建 socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(1);
    }
    
    // 设置端口复用
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // 绑定地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(1);
    }
    
    // 监听
    if (listen(server_fd, 5) < 0) {
        perror("listen failed");
        exit(1);
    }
    
    printf("[Server] Listening on port %d...\n", PORT);
    
    // 接受客户端连接
    while (1) {
        int *client_fd = malloc(sizeof(int));
        if (client_fd == NULL) {
            perror("malloc failed");
            continue;
        }
        *client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        
        if (*client_fd < 0) {
            perror("accept failed");
            free(client_fd);
            continue;
        }
        
        printf("[Server] Client connected from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        // 创建线程处理客户端
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, client_fd) != 0) {
            perror("pthread_create failed");
            close(*client_fd);
            free(client_fd);
            continue;
        }
        pthread_detach(tid);
    }
    
    close(server_fd);
    return 0;
}
