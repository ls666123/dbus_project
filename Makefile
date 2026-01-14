CC = gcc
CFLAGS = -Wall -pthread -I./src
LDFLAGS = -lpthread -lsqlite3

all: server client test_database

server: src/server.c
	$(CC) $(CFLAGS) -o server src/server.c $(LDFLAGS)

client: src/client.c
	$(CC) $(CFLAGS) -o client src/client.c

test_database: src/test_database.c src/database.c
	$(CC) $(CFLAGS) -o test_database src/test_database.c src/database.c $(LDFLAGS)

clean:
	rm -f server client test_database *.db

.PHONY: all clean
