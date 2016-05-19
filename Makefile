CC = gcc
CFLAGS = -Wall - g

all: run
 
run: bankServer.c clientConnect.c sorted-list.o
	$(CC) $(CCFLAGS) -pthread -o bankserver bankServer.c sorted-list.o
	$(CC) $(CCFLAGS) -pthread -o client clientConnect.c

sorted-list.o: sorted-list.c sorted-list.h
	$(CC) -c sorted-list.c

debug: sorted-list.o bankServer.c
	gcc —pthread -g sorted-list.o bankServer.c

clean:
	rm -rf a.out
	rm -rf *.o
	rm -rf bankserver
	rm -rf client
