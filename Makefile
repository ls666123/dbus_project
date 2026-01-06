CC = gcc
CFLAGS = -Wall -pthread

all: server client

server: src/server.c
	$(CC) $(CFLAGS) -o server src/server.c -lpthread

client: src/client.c
	$(CC) $(CFLAGS) -o client src/client.c

clean:
	rm -f server client
