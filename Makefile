CC = gcc
WALL = -Wall
CFLAGS = $(WALL)

all: chat

chat: chat.o
	$(CC) $(CFLAGS) -o chat chat.o

chat.o: chat.c
	$(CC) $(CFLAGS) -c chat.c

clean:
	rm -f chat chat.o

