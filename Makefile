nano README.md             # 删掉旧内容，粘贴新的 README.mdCC = gcc
CFLAGS = -Wall -Wextra -g -pthread -I./src
LDFLAGS_SERVER = -lpthread -lsqlite3
LDFLAGS_DB = -lsqlite3

all: server client database
	@echo "Build complete!"

server: src/server.c src/database.c src/database.h
	$(CC) $(CFLAGS) -o $@ src/server.c src/database.c $(LDFLAGS_SERVER)
	@echo "[OK] Server"

client: src/client.c
	$(CC) $(CFLAGS) -o $@ src/client.c
	@echo "[OK] Client"

database: src/database.c src/database.h
	$(CC) $(CFLAGS) -DTEST_MODE -o $@ src/database.c $(LDFLAGS_DB)
	@echo "[OK] Database"

test: database
	./database test

clean:
	rm -f server client database *.db

.PHONY: all test clean
CC = gcc
CFLAGS = -Wall -Wextra -g -pthread -I./src
LDFLAGS_SERVER = -lpthread -lsqlite3
LDFLAGS_DB = -lsqlite3

# DBus 编译参数 (使用 pkg-config 获取 sd-bus 头文件和库路径)
DBUS_CFLAGS = $(shell pkg-config --cflags libsystemd)
DBUS_LIBS = $(shell pkg-config --libs libsystemd)

all: server client database
	@echo "Build complete!"

# ==================== 第一~三部分 ====================

server: src/server.c src/database.c src/database.h
	$(CC) $(CFLAGS) -o $@ src/server.c src/database.c $(LDFLAGS_SERVER)
	@echo "[OK] Server"

client: src/client.c
	$(CC) $(CFLAGS) -o $@ src/client.c
	@echo "[OK] Client"

database: src/database.c src/database.h
	$(CC) $(CFLAGS) -DTEST_MODE -o $@ src/database.c $(LDFLAGS_DB)
	@echo "[OK] Database"

# ==================== 加分项: DBus IPC ====================

dbus: dbus_service server_dbus
	@echo "[OK] DBus IPC build complete!"

# DBus 服务端: 拥有数据库，通过 DBus 暴露 Insert/Select 方法
dbus_service: src/dbus_service.c src/database.c src/database.h src/dbus_service.h
	$(CC) $(CFLAGS) $(DBUS_CFLAGS) -o $@ src/dbus_service.c src/database.c $(DBUS_LIBS) $(LDFLAGS_DB)
	@echo "[OK] DBus Service"

# 使用 DBus IPC 的 Socket 服务器
server_dbus: src/server_dbus.c src/dbus_client.c src/dbus_client.h src/dbus_service.h
	$(CC) $(CFLAGS) $(DBUS_CFLAGS) -o $@ src/server_dbus.c src/dbus_client.c $(DBUS_LIBS) -lpthread
	@echo "[OK] Server (DBus)"

# ==================== 测试 ====================

test: database
	./database test

# ==================== 清理 ====================

clean:
	rm -f server client database dbus_service server_dbus *.db

.PHONY: all dbus test clean
