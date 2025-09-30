CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread

all: client server

client: client.c same.o
	$(CC) $(CFLAGS) -o client client.c same.o

server: server.c same.o
	$(CC) $(CFLAGS) -o server server.c same.o

common.o: same.c same.h
	$(CC) $(CFLAGS) -c same.c

servers.txt:
	echo "127.0.0.1:20001" > servers.txt
	echo "127.0.0.1:20002" >> servers.txt
	echo "127.0.0.1:20003" >> servers.txt

test: client server servers.txt
	@echo "=== Запуск серверов ==="
	@./server --port 20001 --tnum 2 &
	@./server --port 20002 --tnum 2 &
	@./server --port 20003 --tnum 2 &
	@sleep 2
	@echo "=== Тестирование клиента ==="
	@./client --k 10 --mod 1000 --servers servers.txt
	@./client --k 5 --mod 100 --servers servers.txt
	@./client --k 15 --mod 10000 --servers servers.txt
	@echo "=== Остановка серверов ==="
	@pkill -f "./server --port" || true

clean:
	rm -f client server servers.txt same.o

.PHONY: all clean test