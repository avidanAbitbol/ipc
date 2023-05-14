CC = gcc
WALL = -Wall
CFLAGS = -g -O2 $(WALL)

all: stnc

stnc: stnc.o mainServer.o
	$(CC) $(CFLAGS) -o stnc stnc.o

stnc.o: stnc.c
	$(CC) $(CFLAGS) -c stnc.c

mainServer: mainServer.o
	$(CC) $(CFLAGS) -o mainServer mainServer.o

mainServer.o: mainServer.c
	$(CC) $(CFLAGS) -c mainServer.c


#mainClient: mainClient.o
#	$(CC) $(CFLAGS) -o mainClient mainClient.o

#mainClient.o: mainClient.c
#	$(CC) $(CFLAGS) -c mainClient.c

clean:
	rm -f stnc stnc.o mainServer mainServer.o

