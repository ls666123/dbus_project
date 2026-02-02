CC = gcc
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
