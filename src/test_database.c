#include <stdio.h>
#include <string.h>
#include "database.h"

int main() {
    // 初始化数据库
    if (database_init("messages.db") < 0) {
        fprintf(stderr, "Failed to init database\n");
        return 1;
    }

    // 测试插入
    printf("\n=== Testing database_insert ===\n");
    database_insert("Receive", "Hello from client");
    database_insert("Return", "Server received: Hello from client");
    database_insert("Receive", "Test message 123");
    database_insert("Return", "Server received: Test message 123");

    // 测试查询
    printf("\n=== Testing database_select ===\n");
    char result[4096];
    int count = database_select(result, sizeof(result));
    
    printf("\nQuery result (%d records):\n", count);
    printf("ID|Type|Data|Timestamp\n");
    printf("------------------------\n");
    printf("%s", result);

    // 关闭数据库
    database_close();

    return 0;
}
