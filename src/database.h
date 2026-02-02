#ifndef DATABASE_H
#define DATABASE_H

// 初始化数据库
int database_init(const char *db_path);

// 关闭数据库
void database_close(void);

// 插入数据
int database_insert(const char *type, const char *data);

// 查询数据
int database_select(char *result, int result_size);

// 清空所有数据
int database_clear(void);

// 获取记录总数
int database_count(void);

#endif
