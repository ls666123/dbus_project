#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>
#include "database.h"

static sqlite3 *db = NULL;

// 获取当前时间字符串
static void get_current_time(char *time_str, int size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(time_str, size, "%Y-%m-%d %H:%M:%S", t);
}

// 初始化数据库
int database_init(const char *db_path) {
    int rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    // 创建消息表
    const char *sql = 
        "CREATE TABLE IF NOT EXISTS messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "type TEXT NOT NULL,"
        "data TEXT NOT NULL,"
        "timestamp TEXT NOT NULL);";
    
    char *err_msg = NULL;
    rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }

    printf("[Database] Initialized successfully: %s\n", db_path);
    return 0;
}

// 关闭数据库
void database_close(void) {
    if (db) {
        sqlite3_close(db);
        db = NULL;
        printf("[Database] Closed.\n");
    }
}

// 插入数据
int database_insert(const char *type, const char *data) {
    if (!db) {
        fprintf(stderr, "Database not initialized\n");
        return -1;
    }

    char time_str[64];
    get_current_time(time_str, sizeof(time_str));

    // 使用预处理语句防止 SQL 注入
    const char *sql = "INSERT INTO messages (type, data, timestamp) VALUES (?, ?, ?);";
    sqlite3_stmt *stmt;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, type, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, data, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, time_str, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Insert failed: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);
    printf("[Database] Inserted: type=%s, data=%s, time=%s\n", type, data, time_str);
    return 0;
}

// 查询数据
int database_select(char *result, int result_size) {
    if (!db) {
        fprintf(stderr, "Database not initialized\n");
        return -1;
    }

    const char *sql = "SELECT id, type, data, timestamp FROM messages ORDER BY id DESC LIMIT 20;";
    sqlite3_stmt *stmt;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    result[0] = '\0';
    int count = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char *type = (const char *)sqlite3_column_text(stmt, 1);
        const char *data = (const char *)sqlite3_column_text(stmt, 2);
        const char *timestamp = (const char *)sqlite3_column_text(stmt, 3);

        char line[256];
        snprintf(line, sizeof(line), "%d|%s|%s|%s\n", id, type, data, timestamp);
        
        if (strlen(result) + strlen(line) < (size_t)result_size) {
            strcat(result, line);
            count++;
        }
    }

    sqlite3_finalize(stmt);
    printf("[Database] Selected %d records.\n", count);
    return count;
}
