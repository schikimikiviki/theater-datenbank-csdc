CC = gcc
CFLAGS = -g -Wextra -Wall -I/usr/include/postgresql
LDFLAGS = -lpq

server: server.c
	$(CC) $(CFLAGS) -o server server.c database.c $(LDFLAGS)

build: server

run: server
	./server