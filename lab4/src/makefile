CC=gcc
CFLAGS=-I.
all: zombie process_memory

zombie: zombiks.c
	$(CC) -o zombie zombiks.c $(CFLAGS)

process_memory: process_memory.c
	$(CC) -o process_memory process_memory.c $(CFLAGS)


psum: parallel_sum.c utils.c sum.c utils.h sum.h
	$(CC) -Wall -Wextra -std=c99 -pthread -I. -o psum parallel_sum.c utils.c sum.c

clean:
	rm -f zombie process_memory	