#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>
#include "database.h"

static sqlite3 *db = NULL;

static void get_current_time(char *time_str, int size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(time_str, size, "%Y-%m-%d %H:%M:%S", t);
}

int database_init(const char *db_path) {
    if (db != NULL) {
        fprintf(stderr, "[Database] Already initialized\n");
        return 0;
    }

    int rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[Database] Cannot open: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    const char *sql = 
        "CREATE TABLE IF NOT EXISTS messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "type TEXT NOT NULL,"
        "data TEXT NOT NULL,"
        "timestamp TEXT NOT NULL);";

    char *err_msg = NULL;
    rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[Database] SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        db = NULL;
        return -1;
    }

    printf("[Database] Initialized: %s\n", db_path);
    return 0;
}

void database_close(void) {
    if (db != NULL) {
        sqlite3_close(db);
        db = NULL;
        printf("[Database] Closed\n");
    }
}

int database_insert(const char *type, const char *data) {
    if (db == NULL) {
        fprintf(stderr, "[Database] Not initialized\n");
        return -1;
    }

    if (type == NULL || data == NULL) {
        fprintf(stderr, "[Database] Invalid parameters\n");
        return -1;
    }

    char time_str[64];
    get_current_time(time_str, sizeof(time_str));

    const char *sql = "INSERT INTO messages (type, data, timestamp) VALUES (?, ?, ?);";
    sqlite3_stmt *stmt = NULL;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[Database] Prepare failed: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, type, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, data, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, time_str, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "[Database] Insert failed: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);
    printf("[Database] Inserted: type=%s, data=%s, time=%s\n", type, data, time_str);
    return 0;
}

int database_select(char *result, int result_size) {
    if (db == NULL) {
        fprintf(stderr, "[Database] Not initialized\n");
        return -1;
    }

    if (result == NULL || result_size <= 0) {
        fprintf(stderr, "[Database] Invalid parameters\n");
        return -1;
    }

    const char *sql = "SELECT id, type, data, timestamp FROM messages ORDER BY id DESC LIMIT 20;";
    sqlite3_stmt *stmt = NULL;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[Database] Prepare failed: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    result[0] = '\0';
    int count = 0;
    size_t used = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char *type = (const char *)sqlite3_column_text(stmt, 1);
        const char *data = (const char *)sqlite3_column_text(stmt, 2);
        const char *timestamp = (const char *)sqlite3_column_text(stmt, 3);

        if (type == NULL) type = "";
        if (data == NULL) data = "";
        if (timestamp == NULL) timestamp = "";

        char line[512];
        int len = snprintf(line, sizeof(line), "%d|%s|%s|%s\n", id, type, data, timestamp);

        if (used + len < (size_t)result_size) {
            strcat(result, line);
            used += len;
            count++;
        } else {
            break;
        }
    }

    sqlite3_finalize(stmt);
    printf("[Database] Selected %d records\n", count);
    return count;
}

int database_clear(void) {
    if (db == NULL) {
        fprintf(stderr, "[Database] Not initialized\n");
        return -1;
    }

    const char *sql = "DELETE FROM messages;";
    char *err_msg = NULL;

    int rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[Database] Clear failed: %s\n", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }

    printf("[Database] All records cleared\n");
    return 0;
}

int database_count(void) {
    if (db == NULL) {
        fprintf(stderr, "[Database] Not initialized\n");
        return -1;
    }

    const char *sql = "SELECT COUNT(*) FROM messages;";
    sqlite3_stmt *stmt = NULL;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[Database] Prepare failed: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count;
}

/* ======================== 测试模式 ======================== */
#ifdef TEST_MODE

static void print_usage(const char *prog) {
    printf("\nUsage: %s <command> [arguments]\n\n", prog);
    printf("Commands:\n");
    printf("  test                    Run full test\n");
    printf("  insert <type> <data>    Insert a record\n");
    printf("  select                  Query records\n");
    printf("  count                   Show record count\n");
    printf("  clear                   Delete all records\n");
    printf("  help                    Show this help\n\n");
}

static void run_test(void) {
    printf("\n===== Database Test =====\n\n");

    printf("[Test] Inserting records...\n");
    database_insert("Receive", "Hello from client");
    database_insert("Return", "Server received: Hello from client");
    database_insert("Receive", "Test message 123");
    database_insert("Return", "Server received: Test message 123");

    printf("\n[Test] Querying records...\n");
    char result[4096];
    int count = database_select(result, sizeof(result));
    printf("\nRecords (%d):\n", count);
    printf("ID|Type|Data|Timestamp\n");
    printf("------------------------\n");
    printf("%s", result);

    printf("\n[Test] Counting records...\n");
    int total = database_count();
    printf("Total: %d\n", total);

    printf("\n===== Test Complete =====\n\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "help") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    if (database_init("messages.db") < 0) {
        fprintf(stderr, "Failed to initialize database\n");
        return 1;
    }

    int ret = 0;

    if (strcmp(cmd, "test") == 0) {
        run_test();
    }
    else if (strcmp(cmd, "insert") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s insert <type> <data>\n", argv[0]);
            ret = 1;
        } else {
            ret = (database_insert(argv[2], argv[3]) < 0) ? 1 : 0;
        }
    }
    else if (strcmp(cmd, "select") == 0) {
        char result[4096];
        int count = database_select(result, sizeof(result));
        if (count >= 0) {
            printf("\nRecords (%d):\n%s", count, result);
        } else {
            ret = 1;
        }
    }
    else if (strcmp(cmd, "count") == 0) {
        int count = database_count();
        if (count >= 0) {
            printf("Total records: %d\n", count);
        } else {
            ret = 1;
        }
    }
    else if (strcmp(cmd, "clear") == 0) {
        ret = (database_clear() < 0) ? 1 : 0;
    }
    else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        print_usage(argv[0]);
        ret = 1;
    }

    database_close();
    return ret;
}

#endif
